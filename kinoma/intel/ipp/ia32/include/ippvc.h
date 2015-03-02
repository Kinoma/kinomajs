/* ///////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//              Intel(R) Integrated Performance Primitives
//                  Video Coding (ippvc)
//
*/

#if !defined( __IPPVC_H__ ) || defined( _OWN_BLDPCS )
#define __IPPVC_H__

#if defined (_WIN32_WCE) && defined (_M_IX86) && defined (__stdcall)
  #define _IPP_STDCALL_CDECL
  #undef __stdcall
#endif

#ifndef __IPPDEFS_H__
#include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* ////////////////////////////////////////////////////////////////////////////
//                      Structures and definitions                          //
//////////////////////////////////////////////////////////////////////////// */

#if !defined( _OWN_BLDPCS )
/* flags for motion compensation */

#define IPPVC_VLC_FORBIDDEN       0xf0f1
#define IPPVC_ESCAPE              0x00ff
#define IPPVC_ENDOFBLOCK          0x00fe
#define IPPVC_FRAME_PICTURE       0x0003

typedef enum _IPPVC_ESCAPE_FLAG
{
  IPPVC_EF_NONE            = 0x0,
  IPPVC_EF_REVERSIBLE_VLC  = 0x1,
  IPPVC_EF_SHORT_HEADER    = 0x2

} IPPVC_ESCAPE_FLAG;


typedef enum _IPPVC_MC_APX
{
  IPPVC_MC_APX_FF =  0x0,
  IPPVC_MC_APX_FH =  0x4,
  IPPVC_MC_APX_HF =  0x8,
  IPPVC_MC_APX_HH =  0x0c

} IPPVC_MC_APX;



typedef enum _IPPVC_MV_TYPE
{
  IPPVC_MV_FIELD = 0x0,
  IPPVC_MV_FRAME = 0x1

} IPPVC_MV_TYPE;


typedef enum _IppvcFrameFieldFlag
{
  IPPVC_FRAME           = 0x0,
  IPPVC_TOP_FIELD       = 0x1,
  IPPVC_BOTTOM_FIELD    = 0x2

}IppvcFrameFieldFlag;

/* VL code longer than 8 bits */
typedef struct _IppVCHuffmanSpec_32u
{
  Ipp32u code; /* right justified */
  Ipp32u len;
} IppVCHuffmanSpec_32u;


typedef Ipp32s IppVCHuffmanSpec_32s ;


/* Motion Vector */
typedef struct _IppMotionVector
{
  Ipp16s  dx;
  Ipp16s  dy;
} IppMotionVector;


typedef enum
{
  IPP_4x4_VERT     = 0,
  IPP_4x4_HOR      = 1,
  IPP_4x4_DC       = 2,
  IPP_4x4_DIAG_DL  = 3,
  IPP_4x4_DIAG_DR  = 4,
  IPP_4x4_VR       = 5,
  IPP_4x4_HD       = 6,
  IPP_4x4_VL       = 7,
  IPP_4x4_HU       = 8

} IppIntra4x4PredMode_H264;

typedef enum
{
    IPP_8x8_VERT     = 0,
    IPP_8x8_HOR      = 1,
    IPP_8x8_DC       = 2,
    IPP_8x8_DIAG_DL  = 3,
    IPP_8x8_DIAG_DR  = 4,
    IPP_8x8_VR       = 5,
    IPP_8x8_HD       = 6,
    IPP_8x8_VL       = 7,
    IPP_8x8_HU       = 8

} IppIntra8x8PredMode_H264;

typedef enum
{
  IPP_16X16_VERT   = 0,
  IPP_16X16_HOR    = 1,
  IPP_16X16_DC     = 2,
  IPP_16X16_PLANE  = 3

} IppIntra16x16PredMode_H264;


typedef enum
{
  IPP_CHROMA_DC    = 0,
  IPP_CHROMA_HOR   = 1,
  IPP_CHROMA_VERT  = 2,
  IPP_CHROMA_PLANE = 3

} IppIntraChromaPredMode_H264;

enum
{
         IPPVC_LEFT_EDGE                = 0x1,
         IPPVC_RIGHT_EDGE               = 0x2,
         IPPVC_TOP_EDGE                 = 0x4,
         IPPVC_BOTTOM_EDGE              = 0x8,
         IPPVC_TOP_LEFT_EDGE    = 0x10,
         IPPVC_TOP_RIGHT_EDGE   = 0x20
};

#define                 IPPVC_CBP_1ST_CHROMA_DC_BITPOS 17
#define                 IPPVC_CBP_1ST_CHROMA_AC_BITPOS 19
#define                 IPPVC_CBP_CHROMA_DC (0x3<<IPPVC_CBP_1ST_CHROMA_DC_BITPOS)
#define                 IPPVC_CBP_CHROMA_AC (0xff<<IPPVC_CBP_1ST_CHROMA_AC_BITPOS)
#define                 IPPVC_CBP_LUMA_AC (0xffff<<IPPVC_CBP_1ST_LUMA_AC_BITPOS)
#define                 IPPVC_CBP_1ST_LUMA_AC_BITPOS 1
#define                 IPPVC_CBP_LUMA_DC 1
#define                 MAX_CAVLC_LEVEL_VALUE   2063

enum
{
  IPPVC_MBTYPE_INTER        = 0,    /* P picture or P-VOP */
  IPPVC_MBTYPE_INTER_Q      = 1,    /* P picture or P-VOP */
  IPPVC_MBTYPE_INTER4V      = 2,    /* P picture or P-VOP */
  IPPVC_MBTYPE_INTRA        = 3,    /* I and P picture, or I- and P-VOP */
  IPPVC_MBTYPE_INTRA_Q      = 4,    /* I and P picture, or I- and P-VOP */
  IPPVC_MBTYPE_INTER4V_Q    = 5,    /* P picture or P-VOP(H.263)*/
  IPPVC_MBTYPE_DIRECT       = 6,    /* B picture or B-VOP (MPEG-4 only) */
  IPPVC_MBTYPE_INTERPOLATE  = 7,    /* B picture or B-VOP */
  IPPVC_MBTYPE_BACKWARD     = 8,    /* B picture or B-VOP */
  IPPVC_MBTYPE_FORWARD      = 9,    /* B picture or B-VOP */
  IPPVC_MB_STUFFING         = 255
};

enum
{
  IPPVC_SCAN_NONE        = -1,
  IPPVC_SCAN_ZIGZAG      = 0,
  IPPVC_SCAN_VERTICAL    = 1,
  IPPVC_SCAN_HORIZONTAL  = 2
};

/* Block Type */
enum
{
    IPPVC_BLOCK_LUMA   = 0,
    IPPVC_BLOCK_CHROMA = 1
};

/* Interpolation types */
enum
{
  IPPVC_INTERP_NONE = 0,
  IPPVC_INTERP_HORIZONTAL = 1,
  IPPVC_INTERP_VERTICAL = 2,
  IPPVC_INTERP_2D = 3
};

/* Sprite Type */
enum
{
    IPPVC_SPRITE_STATIC = 1,
    IPPVC_SPRITE_GMC    = 2
};

typedef struct WarpSpec_MPEG4 IppiWarpSpec_MPEG4;
typedef struct QuantInvIntraSpec_MPEG4 IppiQuantInvIntraSpec_MPEG4;
typedef struct QuantInvInterSpec_MPEG4 IppiQuantInvInterSpec_MPEG4;
typedef struct QuantIntraSpec_MPEG4 IppiQuantIntraSpec_MPEG4;
typedef struct QuantInterSpec_MPEG4 IppiQuantInterSpec_MPEG4;

/* General Color Conversion Enumerated Types */
enum {
    IPPVC_ROTATE_DISABLE = 0,
    IPPVC_ROTATE_90CCW   = 1,
    IPPVC_ROTATE_90CW    = 2,
    IPPVC_ROTATE_180     = 3
};
enum
{
    IPPVC_CbYCr422ToBGR565 = 0,
    IPPVC_CbYCr422ToBGR555 = 1
};
#endif /* _OWN_BLDPCS */




/* ///////////////////////////////////////////////////////////////////////////
//  Name:       ippvcGetLibVersion
//  Purpose:    getting of the library version
//  Returns:    the structure of information about  version of ippvc library
//  Parameters:
//
//  Notes:      not necessary to release the returned structure
*/

IPPAPI( const IppLibraryVersion*, ippvcGetLibVersion, (void) )



/* ///////////////////////////////////////////////////////////////////////////
//                     MPEG-1 and MPEG-2 Video Decoding Functions
//////////////////////////////////////////////////////////////////////////// */


/* Variable Length Decoding Functions */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiHuffmanTableInitAlloc_32s
//
//  Purpose:
//    allocates memory and initializes the table that contains codes
//    for macroblock address increment, macroblock type, macroblock pattern,
//    or motion vectors.
//
//  Parameters:
//    pSrcTable   Pointer to the source table
//    ppDstSpec   Pointer to pointer to the destination decoding table
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//        ippStsMemAllocErr No memory is allocated.
*/

IPPAPI(IppStatus, ippiHuffmanTableInitAlloc_32s, (
  const Ipp32s*                pSrcTable,
        IppVCHuffmanSpec_32s** ppDstSpec))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiHuffmanRunLevelTableInitAlloc_32s
//
//  Purpose:
//    Allocates memory and initializes the table that contains Run-Level codes.
//
//  Parameters:
//    pSrcTable   Pointer to the source table
//    ppDstSpec   Pointer to pointer to the destination decoding table
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//        ippStsMemAllocErr No memory is allocated.
*/

IPPAPI(IppStatus, ippiHuffmanRunLevelTableInitAlloc_32s, (
  const Ipp32s*                pSrcTable,
        IppVCHuffmanSpec_32s** ppDstSpec))


/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiDecodeHuffmanOne_1u32s
//
// Purpose:
//   Decodes one code using a specified table
//
// Parameters:
//   ppBitStream   Double pointer to the current position in the bit stream
//   pOffset       Pointer to offset between the bit pointed by pBitStream
//                 and the start of the code
//   pDst          Pointer to the destination result
//   pDecodeTable  Pointer to the decoding table
//
// Returns:
//   ippStsNoErr           No error
//   ippStsNullPtrErr      One of the pointers is NULL
//   ippStsH263VLCCodeErr  Decoding in accordance with H.263 Standard
*/

IPPAPI(IppStatus, ippiDecodeHuffmanOne_1u32s, (
        Ipp32u**              ppBitStream,
        int*                  pOffset,
        Ipp32s*               pDst,
  const IppVCHuffmanSpec_32s* pDecodeTable))

/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiDecodeHuffmanPair_1u16s
//
// Purpose:
//   Decodes one code using a specified table
//
// Parameters:
//   ppBitStream   Double pointer to the current position in the bit stream
//   pOffset       Pointer to offset between the bit pointed by pBitStream
//                 and the start of the code
//   pDecodeTable  Pointer to the decoding table
//   pFirst       Pointer to the first destination result
//   pSecond      Pointer to the second destination result
//
// Returns:
//   ippStsNoErr           No error
//   ippStsNullPtrErr      One of the pointers is NULL
//   ippStsH263VLCCodeErr  Decoding in accordance with H.263 Standard
*/

IPPAPI(IppStatus, ippiDecodeHuffmanPair_1u16s, (
        Ipp32u **ppBitStream,
        Ipp32s *pOffset,
  const IppVCHuffmanSpec_32s *pDecodeTable,
        Ipp8s *pFirst,
        Ipp16s *pSecond))

/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiReconstructDCTBlock_MPEG1_32s
//
// Purpose:
//   Decodes 8x8 non-intra block using a table with Run-Level codes
//   for MPEG-1 Standard, rearranges and performs inverse quantization.
//
// Parameters:
//   ppBitStream    Double pointer to the current position in the bitstream.
//   pOffset        Pointer to offset between the bit pointed by pBitStream
//                  and the start of the code
//   pDCSizeTable   Pointer to the table with DC coefficient,
//                  that is the first of the DCT coefficients
//   pACTable       Pointer to the table with Run-Level codes for
//                  all DCT coefficients but the first
//   pScanMatrix    Pointer to the matrix containing indices of elements
//                  in scanning sequence
//   QP             Quantizer scale factor which is read from the bitstream
//   pQPMatrix      Pointer to the weighting matrix imposed by the Standard
//                  or user-defined
//   pDstBlock      Pointer to the decoded elements
//   pDstSize       Pointer to the position of the last non-zero block coefficient
//                  in scanning sequence
//
// Returns:
//   ippStsNoErr           No error
//   ippStsH263VLCCodeErr  Decoding in accordance with H.263 Standard
*/

IPPAPI(IppStatus, ippiReconstructDCTBlock_MPEG1_32s, (
        Ipp32u** ppBitStream,
        int*     pOffset,
  const Ipp32s*  pDCSizeTable,
  const Ipp32s*  pACTable,
        Ipp32s*  pScanMatrix,
        int      QP,
        Ipp16s*  pQPMatrix,
        Ipp16s*  pDstBlock,
        Ipp32s*     pDstSize))


/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiReconstructDCTBlockIntra_MPEG1_32s
//
// Purpose:
//   Decodes 8x8 intra block using a table with Run-Level codes
//   for MPEG-1 Standard, rearranges and performs inverse quantization.
//
// Parameters:
//   ppBitStream    Double pointer to the current position in the bitstream.
//   pOffset        Pointer to offset between the bit pointed by pBitStream
//                  and the start of the code
//   pDCSizeTable   Pointer to the table with codes for DC coefficient,
//                  that is the first of the DCT coefficients
//   pACTable       Pointer to the table with Run-Level codes for
//                  all DCT coefficients but the first
//   pScanMatrix    Pointer to the scanning matrix imposed by the Standard
//                  or user-defined
//   QP             Quantizer scale factor which is read from the bitstream
//   pQPMatrix      Pointer to the weighting matrix imposed by the Standard
//                  or user-defined
//   pDCPred        Pointer to the value to be added to the DC coefficient
//   pDstBlock      Pointer to the decoded elements
//   pDstSize       Pointer to the position of the last non-zero block
//                  coefficient in scanning sequence
//
// Returns:
//   ippStsNoErr           No error
//   ippStsH263VLCCodeErr  Decoding in accordance with H.263 Standard
*/

IPPAPI(IppStatus, ippiReconstructDCTBlockIntra_MPEG1_32s, (
        Ipp32u** ppBitStream,
        int*     pOffset,
  const Ipp32s*  pDCSizeTable,
  const Ipp32s*  pACTable,
        Ipp32s*  pScanMatrix,
        int      QP,
        Ipp16s*  pQPMatrix,
        Ipp16s*  pDCPred,
        Ipp16s*  pDstBlock,
        Ipp32s*  pDstSize))


/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiReconstructDCTBlock_MPEG2_32s
//
// Purpose:
//   Decodes 8x8 non-intra block using a table with Run-Level codes for
//   MPEG-2 Standard, rearranges and performs inverse quantization.
//
// Parameters:
//   ppBitStream    Double pointer to the current position in the bitstream.
//   pOffset        Pointer to offset between the bit pointed by pBitStream
//                  and the start of the code
//   pDCTable       Pointer to the table with codes for DC coefficient,
//                  that is the first of the DCT coefficients
//   pACTable       Pointer to the table with Run-Level codes for
//                  all DCT coefficients but the first
//   pScanMatrix    Pointer to the matrix containing indices of elements
//                  in scanning sequence
//   QP             Quantizer scale factor which is read from the bitstream
//   pQPMatrix      Pointer to the weighting matrix imposed by the Standard
//                  or user-defined
//   pDstBlock      Pointer to the decoded elements
//   pDstSize       Pointer to the position of the last non-zero block
//                  coefficient in scanning sequence
//
// Returns:
//   ippStsNoErr           No error
//   ippStsH263VLCCodeErr  Decoding in accordance with H.263 Standard
*/

IPPAPI(IppStatus, ippiReconstructDCTBlock_MPEG2_32s, (
        Ipp32u**              ppBitStream,
        int*               pOffset,
  const IppVCHuffmanSpec_32s* pDCTable,
  const IppVCHuffmanSpec_32s* pACTable,
        Ipp32s*               pScanMatrix,
        int                   QP,
        Ipp16s*               pQPMatrix,
        Ipp16s*               pDstBlock,
        Ipp32s*               pDstSize))


/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiReconstructDCTBlockIntra_MPEG2_32s
//
// Purpose:
//   Decodes 8x8 intra block using a table with Run-Level codes for
//   MPEG-1 Standard, rearranges and performs inverse quantization.
//
// Parameters:
//   ppBitStream     Double pointer to the current position in the bitstream.
//   pOffset        Pointer to offset between the bit pointed by pBitStream
//                  and the start of the code
//   pDCSizeTable   Pointer to the table with codes for DC coefficient,
//                  that is the first of the DCT coefficients
//   pACTable       Pointer to the table with Run-Level codes for
//                  all DCT coefficients but the first
//   pScanMatrix    Pointer to the scanning matrix imposed by the Standard
//                  or user-defined
//   QP             Quantizer scale factor which is read from the bitstream
//   pQPMatrix      Pointer to the weighting matrix imposed by the Standard
//                  or user-defined
//   pDCPred        Pointer to the value to be added to the DC coefficient
//   shiftDCVal     Integer value, DC coefficient must be multiplied by
//                  2**shiftDCVal
//   pDstBlock      Pointer to the decoded elements
//   pDstSize       Pointer to the position of the last non-zero block
//                  coefficient in scanning sequence
//
// Returns:
//   ippStsNoErr           No error
//   ippStsH263VLCCodeErr  Decoding in accordance with H.263 Standard
*/

IPPAPI(IppStatus, ippiReconstructDCTBlockIntra_MPEG2_32s, (
        Ipp32u**              ppBitStream,
        int*                  pOffset,
  const IppVCHuffmanSpec_32s* pDCSizeTable,
  const IppVCHuffmanSpec_32s* pACTable,
        Ipp32s*               pScanMatrix,
        int                   QP,
        Ipp16s*               pQPMatrix,
        Ipp16s*               pDCPred,
        Ipp32s                shiftDCVal,
        Ipp16s*               pDstBlock,
        Ipp32s*               pDstSize))


/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiHuffmanTableFree_32s
//
// Purpose:
//   Frees memory allocated for VLC table
//
// Parameters:
//   pDecodeTable   Pointer to the allocated table
//
// Returns:
//   ippStsNoErr           No error
//   ippStsNullPtrErr      One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiHuffmanTableFree_32s, (IppVCHuffmanSpec_32s *pDecodeTable))




/* Inverse Quantization */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInvIntra_MPEG2_16s_C1I
//    ippiQuantInv_MPEG2_16s_C1I
//
//  Purpose:
//    Performs inverse quantization for intra and non-intra frames
//    respectively in accordance with the MPEG-2 Standard
//
//  Parameters:
//    pSrcDst       Pointer to the block of DCT coefficients
//    QP            Quantizer scale factor which is read from the bitstream
//    pQPMatrix     Pointer to the weighting matrix imposed by the Standard
//                  or user-defined
//
//  Returns:
//   ippStsNoErr           No error
//   ippStsNullPtrErr      One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiQuantInvIntra_MPEG2_16s_C1I, (
  Ipp16s* pSrcDst,
  int     QP,
  Ipp16s* pQPMatrix))

IPPAPI(IppStatus, ippiQuantInv_MPEG2_16s_C1I, (
  Ipp16s* pSrcDst,
  int     QP,
  Ipp16s* pQPMatrix))




/* Inverse Discrete Cosine Transformation */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDCT8x8Inv_AANTransposed_16s_C1R
//
//  Purpose:
//    Performs inverse DCT on a pre-transposed block
//
//  Parameters:
//    pSrc     Pointer to the block of DCT coefficients
//    pDst     Pointer to the destination array
//    dstStep  Step through the destination array
//    count    Number of the last non-zero coefficient in zig-zag order
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Notes:
//    This function is used for non-intra macroblocks
*/

IPPAPI(IppStatus,ippiDCT8x8Inv_AANTransposed_16s_C1R, (
  const Ipp16s* pSrc,
        Ipp16s* pDst,
        Ipp32s  dstStep,
        Ipp32s  count))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDCT8x8Inv_AANTransposed_16s8u_C1R
//
//  Purpose:
//    Performs inverse DCT on a pre-transposed block and
//    converts output to unsigned char format
//
//  Parameters:
//    pSrc     Pointer to the block of DCT coefficients
//    pDst     Pointer to the destination array
//    dstStep  Step through the destination array
//    count    Number of the last non-zero coefficient in zig-zag order
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Notes:
//    This function is used for non-intra macroblocks
*/

IPPAPI(IppStatus,ippiDCT8x8Inv_AANTransposed_16s8u_C1R, (
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        Ipp32s  dstStep,
        Ipp32s  count))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDCT8x8Inv_AANTransposed_16s_P2C2R
//
//  Purpose:
//    Performs Inverse DCT on pre-transposed data of two input chroma blocks
//    and joins the output data into one array
//
//  Parameters:
//    pSrcU      Pointer to block of DCT coefficients for U component
//    pSrcV      Pointer to block of DCT coefficients for V component
//    pDstUV     Pointer to the destination array
//    dstStep    Step through the destination array
//    countU     Number of the last non-zero U coefficient in zig-zag order
//    countV     Number of the last non-zero V coefficient in zig-zag order
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Notes:
//    This function is used for non-intra macroblocks
*/

IPPAPI(IppStatus,ippiDCT8x8Inv_AANTransposed_16s_P2C2R, (
  const Ipp16s* pSrcU,
  const Ipp16s* pSrcV,
        Ipp16s* pDstUV,
        Ipp32s  dstStep,
        Ipp32s  countU,
        Ipp32s  countV))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDCT8x8Inv_AANTransposed_16s8u_P2C2R
//
//  Purpose:
//    Performs inverse DCT on pre-transposed data of two input chroma
//    blocks and joins the output data into one unsigned char array
//
//  Parameters:
//    pSrcU      Pointer to block of DCT coefficients for U component
//    pSrcV      Pointer to block of DCT coefficients for V component
//    pDstUV     Pointer to the destination array
//    dstStep    Step through the destination array
//    countU     Number of the last non-zero U coefficient in zig-zag order
//    countV     Number of the last non-zero V coefficient in zig-zag order
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Notes:
//    This function is used for non-intra macroblocks
*/

IPPAPI(IppStatus, ippiDCT8x8Inv_AANTransposed_16s8u_P2C2R, (
  const Ipp16s* pSrcU,
  const Ipp16s* pSrcV,
        Ipp8u*  pDstUV,
        Ipp32s  dstStep,
        Ipp32s  countU,
        Ipp32s  countV))




/* Motion Compensation */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiMC16x16_8u_C1,  ippiMC16x8_8u_C1,  ippiMC8x16_8u_C1,
//    ippiMC8x8_8u_C1,    ippiMC8x4_8u_C1,   ippiMC4x8_8u_C1,
//    ippiMC4x4_8u_C1,    ippiMC2x4_8u_C1,   ippiMC4x2_8u_C1,
//    ippiMC2x2_8u_C1
//
//  Purpose:
//    Performs motion compensation for a predicted block of
//    the correspondent size
//
//  Parameters:
//    pSrcRef        Pointer to the reference intra block
//    srcStep        Step in bytes, specifying the width of
//                   the aligned reference frame
//    pSrcYData      Pointer to the data obtained after inverse DCT
//    srcYDataStep   Step in bytes, specifying the width of
//                   the aligned data after inverse DCT
//    pDst           Pointer to the destination predicted block
//    dstStep        Step in bytes, specifying the width of
//                   the aligned destination frame
//    mcType         Type of motion compensation, IPPVC_MC_APX
//    roundControl   Type of rounding for half-pixel approximation;
//                   may be 0 or 1
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiMC16x16_8u_C1,(
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC16x8_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC8x16_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC8x8_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC8x4_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC4x8_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC4x4_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC2x4_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC4x2_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC2x2_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       mcType,
        Ipp32s       roundControl))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiMC16x4_8u_C1
//    ippiMC16x8UV_8u_C1
//
//  Purpose:
//    Performs motion compensation for a predicted UV block of
//    the correspondent size
//
//  Parameters:
//    pSrcRef        Pointer to the reference block
//    srcStep        Step in bytes, specifying the width of
//                   the aligned reference frame
//    pSrcYData      Pointer to the data obtained after inverse DCT
//    srcYDataStep   Step in bytes, specifying the width of
//                   the aligned data after inverse DCT
//    pDst           Pointer to the destination predicted block
//    dstStep        Step in bytes, specifying the width of
//                   the aligned destination frame
//    mcType         Type of motion compensation, IPPVC_MC_APX
//    roundControl   Type of rounding for half-pixel approximation;
//                   may be 0 or 1
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiMC16x4_8u_C1, (
  const Ipp8u*  pSrcRef,
        Ipp32s  srcStep,
  const Ipp16s* pSrcYData,
        Ipp32s  srcYDataStep,
        Ipp8u*  pDst,
        Ipp32s  dstStep,
        Ipp32s  mcType,
        Ipp32s  roundControl))

IPPAPI(IppStatus, ippiMC16x8UV_8u_C1, (
  const Ipp8u*       pSrcRef,
        Ipp32s       srcStep,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s   mcType,
        Ipp32s       roundControl))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiMC16x16B_8u_C1,  ippiMC16x8B_8u_C1,  ippiMC8x16B_8u_C1,
//    ippiMC8x8B_8u_C1,    ippiMC8x4B_8u_C1,   ippiMC4x8B_8u_C1,
//    ippiMC4x4B_8u_C1,    ippiMC2x4B_8u_C1,   ippiMC4x2B_8u_C1,
//    ippiMC2x2B_8u_C1
//
//  Purpose:
//    Performs motion compensation for a bi-predicted block of
//    the correspondent size
//
//  Parameters:
//    pSrcRefB       Pointer to the forward reference block
//    srcStepF       Step in bytes, specifying the width of
//                   the aligned forward reference frame
//    mcTypeF        Forward motion compensation type, IPPVC_MC_APX
//    pSrcRefB       Pointer to the backward reference block
//    srcStepB       Step in bytes, specifying the width of
//                   the aligned backward reference frame
//    mcTypeB        Backward motion compensation type, IPPVC_MC_APX
//    pSrcYData      Pointer to the data obtained after inverse DCT
//    srcYDataStep   Step in bytes, specifying the width of
//                   the aligned data after inverse DCT
//    pDst           Pointer to the destination predicted block
//    dstStep        Step in bytes, specifying the width of
//                   the aligned destination frame
//    roundControl   Type of rounding for half-pixel approximation;
//                   may be 0 or 1
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiMC16x16B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC16x8B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC8x16B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC8x8B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC8x4B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC4x8B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC4x4B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC2x4B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC4x2B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC2x2B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiMC16x4B_8u_C1
//    ippiMC16x8BUV_8u_C1
//
//  Purpose:
//    Performs motion compensation for a bi-predicted UV block of
//    the correspondent size
//
//  Parameters:
//    pSrcRefB       Pointer to the forward reference block
//    srcStepF       Step in bytes, specifying the width of
//                   the aligned forward reference frame
//    mcTypeF        Forward motion compensation type, IPPVC_MC_APX
//    pSrcRefB       Pointer to the backward reference block
//    srcStepB       Step in bytes, specifying the width of
//                   the aligned backward reference frame
//    mcTypeB        Backward motion compensation type, IPPVC_MC_APX
//    pSrcYData      Pointer to the data obtained after inverse DCT
//    srcYDataStep   Step in bytes, specifying the width of
//                   the aligned data after inverse DCT
//    pDst           Pointer to the destination predicted block
//    dstStep        Step in bytes, specifying the width of
                     the aligned destination frame
//    roundControl   Type of rounding for half-pixel approximation;
//                   may be 0 or 1
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiMC16x4B_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiMC16x8BUV_8u_C1, (
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcStepB,
        Ipp32s       mcTypeB,
  const Ipp16s*      pSrcYData,
        Ipp32s       srcYDataStep,
        Ipp8u*       pDst,
        Ipp32s       dstStep,
        Ipp32s       roundControl))




/* ///////////////////////////////////////////////////////////////////////////
//     MPEG-1 and MPEG-2 Video Encoding Functions
//////////////////////////////////////////////////////////////////////////// */


/* Motion Estimation and Compensation Functions */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiGetDiff16x16_8u16s_C1
//    ippiGetDiff16x8_8u16s_C1
//    ippiGetDiff8x8_8u16s_C1
//    ippiGetDiff8x16_8u16s_C1
//    ippiGetDiff8x4_8u16s_C1
//
//  Purpose:
//    Evaluates the difference between current predicted and reference blocks
//    of the specified size in accordance with the type of motion compensation.
//
//  Parameters:
//    pSrcCur           Pointer to the current block
//    srcCurStep        Step in bytes of the current block
//    pSrcRef           Pointer to the reference block
//    srcRefStep        Step in bytes of the reference block
//    pDstDiff          Pointer to the destination block containing
//                      differences between current and reference blocks
//    dstDiffStep       Step in bytes through the destination block
//    pDstPredictor     Pointer to the block containing predictors
//    dstPredictorStep  Step in bytes through the predictor block
//    mcType            Type of the following motion compensation
//    roundControl      Type of rounding for half-pixel approximation;
//                      may be 0 or 1
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Notes:
//     These operations are inverse to that performed by correspondent motion
//    compensation functions
*/

IPPAPI(IppStatus, ippiGetDiff16x16_8u16s_C1,(
  const Ipp8u*  pSrcCur,
        Ipp32s  srcCurStep,
  const Ipp8u*  pSrcRef,
        Ipp32s  srcRefStep,
        Ipp16s* pDstDiff,
        Ipp32s  dstDiffStep,
        Ipp16s* pDstPredictor,
        Ipp32s  dstPredictorStep,
        Ipp32s  mcType,
        Ipp32s  roundControl))

IPPAPI(IppStatus, ippiGetDiff16x8_8u16s_C1, (
  const Ipp8u*  pSrcCur,
        Ipp32s  srcCurStep,
  const Ipp8u*  pSrcRef,
        Ipp32s  srcRefStep,
        Ipp16s* pDstDiff,
        Ipp32s  dstDiffStep,
        Ipp16s* pDstPredictor,
        Ipp32s  dstPredictorStep,
        Ipp32s  mcType,
        Ipp32s  roundControl))

IPPAPI(IppStatus, ippiGetDiff8x8_8u16s_C1, (
  const Ipp8u*  pSrcCur,
        Ipp32s  srcCurStep,
  const Ipp8u*  pSrcRef,
        Ipp32s  srcRefStep,
        Ipp16s* pDstDiff,
        Ipp32s  dstDiffStep,
        Ipp16s* pDstPredictor,
        Ipp32s  dstPredictorStep,
        Ipp32s  mcType,
        Ipp32s  roundControl))

IPPAPI(IppStatus, ippiGetDiff8x16_8u16s_C1, (
  const Ipp8u*  pSrcCur,
        Ipp32s  srcCurStep,
  const Ipp8u*  pSrcRef,
        Ipp32s  srcRefStep,
        Ipp16s* pDstDiff,
        Ipp32s  dstDiffStep,
        Ipp16s* pDstPredictor,
        Ipp32s  dstPredictorStep,
        Ipp32s  mcType,
        Ipp32s  roundControl))

IPPAPI(IppStatus, ippiGetDiff8x4_8u16s_C1, (
  const Ipp8u*  pSrcCur,
        Ipp32s  srcCurStep,
  const Ipp8u*  pSrcRef,
        Ipp32s  srcRefStep,
        Ipp16s* pDstDiff,
        Ipp32s  dstDiffStep,
        Ipp16s* pDstPredictor,
        Ipp32s  dstPredictorStep,
        Ipp32s  mcType,
        Ipp32s  roundControl))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiGetDiff4x4_8u16s_C1
//
//  Purpose:
//    Evaluates the difference between current predicted and reference blocks
//    of the specified size in accordance with the type of motion compensation.
//
//  Parameters:
//    pSrcCur           Pointer to the current block
//    srcCurStep        Step in bytes of the current block
//    pSrcRef           Pointer to the reference block
//    srcRefStep        Step in bytes of the reference block
//    pDstDiff          Pointer to the destination block containing
//                      differences between current and reference blocks
//    dstDiffStep       Step in bytes through the destination block
//    pDstPredictor     Reserved parameter (must be 0).
//    dstPredictorStep  Reserved parameter (must be 0).
//    mcType            Reserved parameter (must be 0).
//    roundControl      Reserved parameter (must be 0).
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Notes:
//     These operations are inverse to that performed by correspondent motion
//    compensation functions
*/
IPPAPI( IppStatus,  ippiGetDiff4x4_8u16s_C1,(
  const Ipp8u*  pSrcCur,
        Ipp32s  srcCurStep,
  const Ipp8u*  pSrcRef,
                Ipp32s  srcRefStep,
                Ipp16s* pDstDiff,
                Ipp32s  dstDiffStep,
                Ipp16s* pDstPredictor,
                Ipp32s  dstPredictorStep,
                Ipp32s  mcType,
                Ipp32s  roundControl))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiGetDiff16x16B_8u16s_C1
//
//  Purpose:
//    Evaluates difference between current bi-predicted and mean of two
//    reference blocks of the specified size in accordance with the type of
//    motion compensation.
//
//  Parameters:
//    pSrcCur          Pointer to the current block
//    srcCurStep       Step in bytes through the current block (stride1)
//    pSrcRefF         Pointer to the forward reference block
//    srcRefStepF      Step in bytes through the forward reference block
//                     (stride1)
//    mcTypeF          Forward motion compensation type
//    pSrcRefB         Pointer to the backward reference block
//    srcRefStepB      Step in bytes through the backward reference block
//                     (stride1)
//    mcTypeB          Backward motion compensation type
//    pDstDiff         Pointer to the destination block containing the
//                     differences between current and reference blocks
//    dstDiffStep      Step in bytes through the destination block
//    roundControl     Type of rounding for half-pixel approximation;
//                     may be 0 or 1
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Notes:
//     These operations are inverse to that performed by correspondent motion
//    compensation functions
*/

IPPAPI(IppStatus, ippiGetDiff16x16B_8u16s_C1, (
  const Ipp8u*       pSrcCur,
        Ipp32s       srcCurStep,
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcRefStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcRefStepB,
        Ipp32s       mcTypeB,
        Ipp16s*      pDstDiff,
        Ipp32s       dstDiffStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiGetDiff16x8B_8u16s_C1, (
  const Ipp8u*       pSrcCur,
        Ipp32s       srcCurStep,
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcRefStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcRefStepB,
        Ipp32s       mcTypeB,
        Ipp16s*      pDstDiff,
        Ipp32s       dstDiffStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiGetDiff8x8B_8u16s_C1, (
  const Ipp8u*       pSrcCur,
        Ipp32s       srcCurStep,
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcRefStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcRefStepB,
        Ipp32s       mcTypeB,
        Ipp16s*      pDstDiff,
        Ipp32s       dstDiffStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiGetDiff8x16B_8u16s_C1, (
  const Ipp8u*       pSrcCur,
        Ipp32s       srcCurStep,
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcRefStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcRefStepB,
        Ipp32s       mcTypeB,
        Ipp16s*      pDstDiff,
        Ipp32s       dstDiffStep,
        Ipp32s       roundControl))

IPPAPI(IppStatus, ippiGetDiff8x4B_8u16s_C1, (
  const Ipp8u*       pSrcCur,
        Ipp32s       srcCurStep,
  const Ipp8u*       pSrcRefF,
        Ipp32s       srcRefStepF,
        Ipp32s       mcTypeF,
  const Ipp8u*       pSrcRefB,
        Ipp32s       srcRefStepB,
        Ipp32s       mcTypeB,
        Ipp16s*      pDstDiff,
        Ipp32s       dstDiffStep,
        Ipp32s       roundControl))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSAD16x16_8u32s
//
//  Purpose:
//    Evaluates the sum of absolute difference (SAD) between all elements of
//    the current block and the corresponding elements of the reference block.
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//    ippStsStepErr     srcCurStep or srcRefStep is less than or equal to 0
//
//  Parameters:
//    pSrc           Pointer to the current block
//    srcStep        Step in bytes through the current block
//    pRef           Pointer to the reference block
//    refStep        Step in bytes through the reference block
//    mcType         Type of motion compensation
//    pSAD           Pointer to the result
*/

IPPAPI(IppStatus, ippiSAD16x16_8u32s, (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRef,
        Ipp32s  refStep,
        Ipp32s* pSAD,
        Ipp32s  mcType))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSAD4x4_8u32s
//
//  Purpose:
//    Evaluates the sum of absolute difference (SAD) between all elements of
//    the current block and the corresponding elements of the reference block.
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//    ippStsStepErr     srcCurStep or srcRefStep is less than or equal to 0
//
//  Parameters:
//    pSrc           Pointer to the current block
//    srcStep        Step in bytes through the current block
//    pRef           Pointer to the reference block
//    refStep        Step in bytes through the reference block
//    mcType         reserved and must be 0.
//    pSAD           Pointer to the result
*/

IPPAPI(IppStatus, ippiSAD4x4_8u32s,(const Ipp8u*  pSrc,
                                    Ipp32s        srcStep,
                                    const Ipp8u*  pRef,
                                    Ipp32s        refStep,
                                    Ipp32s*       pSAD,
                                    Ipp32s        mcType ))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//              SAD16x16Blocks8x8_8u16u
//
//  Purpose:
//      Evaluates four partial sums of absolute differences
//      between current and reference 16X16 blocks.
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Parameters:
//              pSrc            Pointer to 16x16 block in the source plane.
//              srcStep         Pitch of the source plane (in bytes).
//              pRef            Pointer to 16x16 block in the reference plane.
//              refStep         Pitch of the reference plane (in bytes).
//              pSAD            Pointer to array of size 4 to store SAD values.
//              mcType          reserved and must be 0.
*/

IPPAPI(IppStatus, ippiSAD16x16Blocks8x8_8u16u,(const   Ipp8u*  pSrc,
                                               Ipp32s  srcStep,
                                               const   Ipp8u*  pRef,
                                               Ipp32s  refStep,
                                               Ipp16u*  pDstSAD,
                                               Ipp32s   mcType ))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//              SAD16x16Blocks4x4_8u16u
//
//  Purpose:
//      Evaluates 16 partial sums of absolute differences
//      between current and reference 16X16 blocks.
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//
//  Parameters:
//              pSrc            Pointer to 16x16 block in the source plane.
//              srcStep         Pitch of the source plane (in bytes).
//              pRef            Pointer to 16x16 block in the reference plane.
//              refStep         Pitch of the reference plane (in bytes).
//              pSAD            Pointer to array of size 16 to store SAD values.
//              mcType          reserved and must be 0.
*/

IPPAPI(IppStatus, ippiSAD16x16Blocks4x4_8u16u, (const   Ipp8u*  pSrc,
                                                Ipp32s  srcStep,
                                                const   Ipp8u*  pRef,
                                                Ipp32s  refStep,
                                                Ipp16u*  pDstSAD,
                                                Ipp32s   mcType))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSqrDiff16x16_8u32s
//
//  Purpose:
//    Evaluates the sum of square differences between all the elements of
//    the current block and correspondent elements of the reference block.
//
//  Parameters:
//    pSrc           Pointer to the current block
//    srcStep        Step in bytes through the current block
//    pRef           Pointer to the reference block
//    refStep        Step in bytes through the reference block
//    mcType         Type of motion compensation
//    pSqrDiff       Pointer to the result
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//    ippStsStepErr     srcCurStep or srcRefStep is less than or equal to 0
*/

IPPAPI(IppStatus, ippiSqrDiff16x16_8u32s, (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRef,
        Ipp32s  refStep,
        Ipp32s  mcType,
        Ipp32s* pSqrDiff))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSqrDiff16x16B_8u32s
//
//  Purpose:
//    Evaluates the sum of square differences between all the elements of
//    the current block and the mean of correspondent elements of
//    two reference blocks.
//
//  Parameters:
//    pSrc          Pointer to the current block
//    srcStep       Step in bytes through the current block
//    pRefF         Pointer to the forward reference block
//    refStepF      Step in bytes through the forward reference block
//    pRefB         Pointer to the backward reference block
//    refStepB      Step in bytes through the backward reference block
//    mcTypeF       Forward motion compensation type
//    mcTypeB       Backward motion compensation type
//    pSqrDiff      Pointer to the result
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//    ippStsStepErr     one of step values is less than or equal to 0
*/

IPPAPI(IppStatus, ippiSqrDiff16x16B_8u32s, (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRefF,
        Ipp32s  refStepF,
        Ipp32s  mcTypeF,
  const Ipp8u*  pRefB,
        Ipp32s  refStepB,
        Ipp32s  mcTypeB,
        Ipp32s* pSqrDiff))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiVariance16x16_8u32s
//
//  Purpose:
//    Evaluates the variance of the current block
//
//  Parameters:
//    pSrc     Pointer to the current block
//    srcStep  Step through the current block
//    pVar     Pointer to the variance
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  pSrc is NULL
//    ippStsStepErr     srcStep is less than or equal to 0
*/

IPPAPI(IppStatus, ippiVariance16x16_8u32s, (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
        Ipp32s* pVar))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiVarMean8x8_8u32s
//
//  Purpose:
//    Evaluates the Variance and mean of 8x8 block of unsigned char values
//
//  Parameters:
//    pSrc       Pointer to the source block
//    srcStep    Step through the source block (stride1)
//    pVar       Pointer to the variance
//    pMean      Pointer to the mean value
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  pSrc is NULL
*/

IPPAPI(IppStatus, ippiVarMean8x8_8u32s_C1R, (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
        Ipp32s* pVar,
        Ipp32s* pMean))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiVarMean8x8_16s32s
//
//  Purpose:
//    Evaluates the variance and mean of 8x8 block of short integer values
//
//  Parameters:
//    pSrc       Pointer to the source block
//    srcStep    Step through the source block (stride1)
//    pVar       Pointer to the variance
//    pMean      Pointer to the mean value
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  pSrc is NULL
*/

IPPAPI(IppStatus, ippiVarMean8x8_16s32s_C1R, (
  const Ipp16s* pSrc,
        Ipp32s  srcStep,
        Ipp32s* pVar,
        Ipp32s* pMean))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEdgesDetect16x16_8u_C1R
//
//  Purpose:
//  This function detects edges inside 16x16 block:
//      finds pair of neighboring (horizontal and vertical) elements with
//  difference is greater than EdgePelDifference.
//  In the case of number of pairs is greater than EdgePelCount,
//  edges are detected and flag (* pRes) is set to 1.
//  Otherwise, edges aren't detected  ((* pRes) is set to 0)
//
//
//  Parameters:
//      pSrc                    Pointer to 16x16 block in current plan
//      srcStep                 Step of the current plan, specifying width of the plane in bytes.
//      EdgePelDifference       The value for estimation of difference between neighboring elements.
//      EdgePelCount            The value for estimation of number of pairs with "big difference"
//      pRes                    Pointers to output value. (*pRes) is equal 1 in
//                              the case of edges are detected and it is equal
//                              0 in the case of edges aren't detected. I
//
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  one of the input pointers is NULL
*/

IPPAPI(IppStatus, ippiEdgesDetect16x16_8u_C1R, (
        const Ipp8u *pSrc,
        Ipp32u srcStep,
        Ipp8u EdgePelDifference,
        Ipp8u EdgePelCount,
        Ipp8u   *pRes
))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiVarMeanDiff16x16_8u32s_C1R
//
//  Purpose:
//    Evaluates the variances and means of four 8x8 blocks containing
//    differences between two 16x16 blocks
//
//  Parameters:
//    pSrc         Pointer to the current block 16x16
//    srcStep      Step in bytes through the current block
//    pRef         Pointer to the reference block 16x16
//    refStep      Step in bytes through the reference block
//    pSrcSum      Pointer to the sum of pixel values for the current block
//    pVar         Pointer to four element array that contains the variances
//    pMean        Pointer to four element array that contains the Mean values
//    mcType       Type of motion compensation
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  one of the input pointers is NULL
*/

IPPAPI(IppStatus, ippiVarMeanDiff16x16_8u32s_C1R, (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRef,
        Ipp32s  refStep,
        Ipp32s* pSrcSum,
        Ipp32s* pVar,
        Ipp32s* pMean,
        Ipp32s  mcType))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiVarMeanDiff16x8_8u32s_C1R
//
//  Purpose:
//    Evaluates the variances and means of four 8x8 blocks containing
//    differences between two 16x8 blocks
//
//  Parameters:
//    pSrc         Pointer to the current block 16x16
//    srcStep      Step in bytes through the current block
//    pRef         Pointer to the reference block 16x16
//    refStep      Step in bytes through the reference block
//    pSrcSum      Pointer to the sum of pixel values for the current block
//    pVar         Pointer to four element array that contains the variances
//    pMean        Pointer to four element array that contains the Mean values
//    mcType       Type of motion compensation
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  one of the input pointers is NULL
*/

IPPAPI(IppStatus, ippiVarMeanDiff16x8_8u32s_C1R, (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRef,
        Ipp32s  refStep,
        Ipp32s* pSrcSum,
        Ipp32s* pVar,
        Ipp32s* pMean,
        Ipp32s  mcType))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSumsDiff16x16Blocks4x4_8u16s_C1, ippiSumsDiff16x16Blocks4x4_8u16s_C1
//
//  Purpose:
//      These functions evaluates difference between current and reference 4x4 blocks
//      and calculates sums of 4x4 residual blocks elements
//  Parameters:
//      pSrc    Pointer  block in current plane
//      srcStep Step of the current plane, specifying width of the plane in bytes.
//      pPred   Pointer to  reference block
//  predStep Step of the reference plane, specifying width of the plane in bytes.
//      pDiff   If it isn't zero, pointer to array  that contains a sequence of 4x4
//      residual blocks.  The array's filled by function if pDiff isn't null.
//      pSums   Pointer to array that contains sums of 4x4 difference blocks coefficients.
//      The array's filled by function.
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  one of the input pointers is NULL
*/
IPPAPI(IppStatus, ippiSumsDiff16x16Blocks4x4_8u16s_C1,
 (
        Ipp8u*          pSrc,
        Ipp32s          srcStep,
        Ipp8u*          pPred,
        Ipp32s          predStep,
        Ipp16s* pSums,
        Ipp16s* pDiff
))

IPPAPI(IppStatus, ippiSumsDiff8x8Blocks4x4_8u16s_C1,
 (
        Ipp8u*          pSrc,
        Ipp32s          srcStep,
        Ipp8u*          pPred,
        Ipp32s          predStep,
        Ipp16s* pSums,
        Ipp16s* pDiff
))

/* Quantization Functions */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantIntra_MPEG2_16s_C1I
//
//  Purpose:
//    Performs quantization on DCT coefficients for intra block in-place with
//    specified quantization matrix according to the MPEG-2 standard.
//    Returns the number of last non-zero coefficient for
//    future considerations. If pointer to inverse quantization matrix is NULL,
//    the default matrix is used
//
//  Parameters:
//    pSrcDst        Pointer to the block of DCT coefficients
//    QP             Quantizer
//    pQPMatrix      Pointer to the matrix of inverted quantization
//                   coefficients (floating point)
//    pCount         Pointer to the position of the last non-zero coefficient
//
//  Returns:
//    ippStsNoErr        No error
//    ippStsNullPtrErr   pSrcDst is NULL
//    ippStsDivByZeroErr QP is equal to 0
*/

IPPAPI(IppStatus, ippiQuantIntra_MPEG2_16s_C1I, (
        Ipp16s* pSrcDst,
        Ipp32s  QP,
  const Ipp32f* pQPMatrix,
        Ipp32s* pCount))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuant_MPEG2_16s_C1I
//
//  Purpose:
//    Performs quantization on DCT coefficients for non-intra block
//    in-place with specified quantization matrix according to the
//    MPEG-2 standard. Returns the number of last non-zero coefficient for
//    future considerations. If pointer to inverse quantization matrix is NULL,
//    the default matrix is used
//
//  Parameters:
//    pSrcDst           Pointer to the block of DCT coefficients
//    QP                Quantizer
//    pQPMatrix         Pointer to the matrix of inverted quantization
//                      coefficients (floating point)
//    pCount            Pointer to the position of the last non-zero coefficient
//
//  Returns:
//    ippStsNoErr        No error
//    ippStsNullPtrErr   pSrcDst is NULL
//    ippStsDivByZeroErr QP is equal to 0
*/

IPPAPI(IppStatus, ippiQuant_MPEG2_16s_C1I, (
        Ipp16s* pSrcDst,
        Ipp32s  QP,
  const Ipp32f* pQPMatrix,
        Ipp32s* pCount))




/* Huffman Encoding Functions */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiCreateRLEncodeTable
//
//  Purpose:
//    Creates Run-Level Encode Table
//
//  Parameters:
//    pSrcTable   Pointer to the source table
//    ppDstSpec    Double pointer to the destination table
//
//  Returns:
//    ippStsNoErr     No error
*/

IPPAPI(IppStatus, ippiCreateRLEncodeTable, (
  const Ipp32s*                pSrcTable,
        IppVCHuffmanSpec_32s** ppDstSpec))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiPutIntraBlock
//
//  Purpose:
//    Encodes, rearranges and puts intra block into bitstream
//
//  Parameters:
//    ppBitStream Double pointer to the current position in the bitstream.
//    pOffset     Pointer to offset between the bit pointed by pBitStream and
//                the start of the code
//    pSrcBlock   Pointer to the block
//    pDCPred     Pointer to the value to be added to the DC coefficient
//    pDCTable    Pointer to the table with codes for DC coefficient,
//                that is the first of the DCT coefficients
//    pACTable    Pointer to the table with Run-Level codes for
//                AC coefficients, that is all DCT coefficients but the first
//    pScanMatrix Pointer to the scanning matrix
//    EOBLen      Length of the block end code
//    EOBCode     Value of the block end code
//    count       Number of the last non-zero coefficient
//
//  Returns:
//    ippStsNoErr     No error
*/

IPPAPI(IppStatus, ippiPutIntraBlock, (
  Ipp32u**              ppBitStream,
  int*                  pOffset,
  Ipp16s*               pSrcBlock,
  Ipp32s*               pDCPred,
  IppVCHuffmanSpec_32u* pDCTable,
  IppVCHuffmanSpec_32s* pACTable,
  Ipp32s*               pScanMatrix,
  Ipp32s                EOBLen,
  Ipp32s                EOBCode,
  Ipp32s                count))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiPutNonIntraBlock
//  Purpose:
//    Encodes, rearranges and puts non-intra block into bitstream
//
//  Parameters:
//    ppBitStream Double pointer to the current position in the bitstream.
//    pOffset     Pointer to offset between the bit pointed by pBitStream and
//                the start of the code
//    pSrcBlock   Pointer to the block
//    pACTable    Pointer to the table with Run-Level codes for
//                AC coefficients, that is all DCT coefficients but the first
//    pScanMatrix Pointer to the scanning matrix
//    EOBLen      Length of the block end code
//    EOBCode     Value of the block end code
//    count       Number of the last non-zero coefficient
//
//  Returns:
//    ippStsNoErr     No error
*/

IPPAPI(IppStatus,  ippiPutNonIntraBlock, (
  Ipp32u**              pBitStream,
  int*                  pOffset,
  Ipp16s*               pSrcBlock,
  IppVCHuffmanSpec_32s* pACTable,
  Ipp32s*               pScanMatrix,
  Ipp32s                EOBLen,
  Ipp32s                EOBCode,
  Ipp32s                count))




/* ///////////////////////////////////////////////////////////////////////////
//            Video Data Decoding Functions for DV
//////////////////////////////////////////////////////////////////////////// */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiInitAllocHuffmanTable_DV_32u
//
//  Purpose:
//    Allocates memory and initializes the table with Run-Level codes for
//    DCT coefficients
//
//  Parameters:
//    pSrcTable1    Pointer to the source table 1
//    pSrcTable2    Pointer to the source table 2
//    ppHuffTable   Double pointer to the destination decoding table
//
//  Returns:
//    ippStsNoErr       No error
//        ippStsMemAllocErr No memory is allocated.
//    ippStsNullPtrErr  One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiInitAllocHuffmanTable_DV_32u, (
  Ipp32s*  pSrcTable1,
  Ipp32s*  pSrcTable2,
  Ipp32u** ppHuffTable))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFreeHuffmanTable_DV_32u
//
//  Purpose:
//    Frees the memory allocated for the VLC table
//
//  Parameters:
//    pHuffTable  Pointer to the decoding table
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  pHuffTable is NULL
*/

IPPAPI(IppStatus, ippiFreeHuffmanTable_DV_32u, (Ipp32u* pHuffTable))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiHuffmanDecodeSegment_DV_8u16s
//
//  Purpose:
//    Decodes and rearranges segment block,
//     multiplies first block element by 128
//
//  Parameters:
//    pStream        Pointer to the bitstream
//    pZigzagTables  Pointer to the array of two scanning matrixes
//    pHuffTable     Pointer to the decoding Huffman Table
//    pBlock         Pointer to decoded elements
//    pBlockParam    Pointer to the output parameters array [30]
//
//  Returns:
//    ippStsNoErr        No error
//    ippStsNullPtrErr   One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiHuffmanDecodeSegment_DV_8u16s, (
  Ipp8u*  pStream,
  Ipp32u* pZigzagTables,
  Ipp32u* pHuffTable,
  Ipp16s* pBlock,
  Ipp32u* pBlockParam))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInv_DV_16s_C1I
//
//  Purpose:
//    Performs inverse quantization on a block
//
//  Parameters:
//    pSrcDst         Pointer to the block
//    pDequantTable   Pointer to the dequantization table
//
//  Returns:
//    ippStsNoErr        No error
//    ippStsNullPtrErr   One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiQuantInv_DV_16s_C1I, (Ipp16s* pSrcDst, Ipp16s* pDequantTable))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDCT2x4x8Inv_16s_C1I
//
//  Purpose:
//    Performs the inverse DCT for block of type 2
//
//  Parameters:
//    pSrcDst   Pointer to the block
//
//  Returns:
//    ippStsNoErr        No error
//    ippStsNullPtrErr   pSrcDst is NULL
*/

IPPAPI(IppStatus, ippiDCT2x4x8Inv_16s_C1I, (Ipp16s* pSrcDst))




/* ///////////////////////////////////////////////////////////////////////////
//            Video Data Encoding Functions for DV
//////////////////////////////////////////////////////////////////////////// */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDCT2x4x8Frw_16s_C1I
//
//  Purpose:
//    Performs DCT for a block of type 2
//
//  Parameters:
//    pSrcDst  Pointer to block
//
//  Returns:
//    ippStsNoErr                No error
//    ippStsNullPtrErr   pSrcDst is NULL
*/

IPPAPI(IppStatus, ippiDCT2x4x8Frw_16s_C1I, (Ipp16s* pSrcDst))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiCountZeros8x8_16s_C1
//
//  Purpose:
//    Evaluates number of zeros in a block
//
//  Parameters:
//    pSrc    Pointer to 8x8 block
//    pCount  Pointer the evaluated number of zeros
//
//  Returns:
//    ippStsNoErr                No error
//    ippStsNullPtrErr   One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiCountZeros8x8_16s_C1, (Ipp16s* pSrc, Ipp32u* pCount))


/* ///////////////////////////////////////////////////////////////////////////
//                             Common Functions
//////////////////////////////////////////////////////////////////////////// */

/* ////////////////////////////////////////////////////////////////////////////
// Name:
//   ippiFrameFieldSAD16x16_8u32s_C1R
//   ippiFrameFieldSAD16x16_16s32s_C1R
// Purpose:  compute SAD between Frame and Field lines
// Returns:
//
//   ippStsNoErr               No error
//   ippStsNullPtrErr          At least one of the pointers is NULL
// Arguments:
//
//   pSrc                      Pointer to the 16x16 block
//   srcStep                   Width of the source plane
//   pFrameSAD                 Pointer to result of Frame SAD
//   pFieldSAD                 Pointer to result of Field SAD
*/

IPPAPI(IppStatus, ippiFrameFieldSAD16x16_8u32s_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp32s*      pFrameSAD,
    Ipp32s*      pFieldSAD))

IPPAPI(IppStatus, ippiFrameFieldSAD16x16_16s32s_C1R, (
    const Ipp16s* pSrc,
    int           srcStep,
    Ipp32s*       pFrameSAD,
    Ipp32s*       pFieldSAD))


/* /////////////////////////////////////////////////////////////////////////////////////////////
// Name:       ippiSAD16x8_8u32s_C1R
// Purpose:    Computes SAD of two blocks 16x8
// Returns:
//
//   ippStsNoErr               No error
//   ippStsNullPtrErr          At least one of the pointers is NULL
// Arguments:
//
//   pSrcCur                   Pointer to the current block
//   srcCurStep                Width of the current plane
//   pSrcRef                   Pointer to the reference block
//   srcRefStep                Width of the reference plane
//   mcType                    Interpolate type
//   pDst                      Pointer to result
*/

IPPAPI(IppStatus, ippiSAD16x8_8u32s_C1R, (
    const Ipp8u  *pSrcCur,
    int           srcCurStep,
    const Ipp8u  *pSrcRef,
    int           srcRefStep,
    Ipp32s       *pDst,
    Ipp32s        mcType))

/* /////////////////////////////////////////////////////////////////////////////////////////////
// Name:       ippiSSD8x8_8u32s_C1R
// Purpose:    compute SSD of two blocks 8x8
// Returns:
//
//   ippStsNoErr               No error
//   ippStsNullPtrErr          At least one of the pointers is NULL
// Arguments:
//
//   pSrcCur                   Pointer to the current block
//   srcCurStep                Width of the current plane
//   pSrcRef                   Pointer to the reference block
//   srcRefStep                Width of the reference plane
//   mcType                    Interpolate type
//   pDst                      Pointer to result
*/

IPPAPI(IppStatus, ippiSSD8x8_8u32s_C1R, (
    const Ipp8u  *pSrcCur,
    int           srcCurStep,
    const Ipp8u  *pSrcRef,
    int           srcRefStep,
    Ipp32s       *pDst,
    Ipp32s        mcType))

/* /////////////////////////////////////////////////////////////////////////////////////////////
// Name:       ippiSSD4x4_8u32s_C1R
// Purpose:    compute SSD of two blocks 4x4
// Returns:
//
//   ippStsNoErr               No error
//   ippStsNullPtrErr          At least one of the pointers is NULL
// Arguments:
//
//   pSrcCur                   Pointer to the current block
//   srcCurStep                Width of the current plane
//   pSrcRef                   Pointer to the reference block
//   srcRefStep                Width of the reference plane
//   mcType                    Interpolate type
//   pDst                      Pointer to result
*/

IPPAPI(IppStatus, ippiSSD4x4_8u32s_C1R, (
    const Ipp8u  *pSrcCur,
    int           srcCurStep,
    const Ipp8u  *pSrcRef,
    int           srcRefStep,
    Ipp32s       *pDst,
    Ipp32s        mcType))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiScanFwd_16s_C1
//
//  Purpose:
//    Performs classical zigzag, alternate-horizontal, or alternate-vertical
//    forward  scan on a block.
//
//  Parameters:
//    pSrc          Pointer to input block (coefficients in the normal order).
//    pDst          Pointer to output block (coefficients in the scan order).
//    countNonZero  Number of non-zero coefficients in the block. Valid within
//                  the range 1 to 64.
//    scan          Type of the scan, takes one of the following values:
//                    IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                    IPPVC_SCAN_HORIZONTAL - alternate-horizontal scan,
//                    IPPVC_SCAN_VERTICAL - alternate-vertical scan
//  Returns:
//    ippStsNoErr         No error.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsOutOfRangeErr Indicates an error condition if countNonZero is out
//                        of the range [1, 64].
//  Notes:
*/
IPPAPI(IppStatus, ippiScanFwd_16s_C1, (
       const Ipp16s* pSrc,
       Ipp16s*       pDst,
       int           countNonZero,
       int           scan))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiScanInv_16s_C1
//
//  Purpose:
//    Performs classical zigzag, alternate-horizontal, or alternate-vertical
//    inverse scan on a block stored in a compact buffer.
//
//  Parameters:
//    pSrc            Pointer to input block (coefficients in the scan order).
//    pDst            Pointer to output block (coefficients in the normal order).
//    indxLastNonZero Index of the last non-zero coefficient. Valid within the
//                    range 0 to 63.
//    scan            Type of the scan, takes one of the following values:
//                     IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                     IPPVC_SCAN_HORIZONTAL - alternate-horizontal scan,
//                     IPPVC_SCAN_VERTICAL - alternate-vertical scan
//  Returns:
//    ippStsNoErr         No error.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsOutOfRangeErr Indicates an error condition if indxLastNonZero is
//                        out of the range [0, 63].
//  Notes:
*/

IPPAPI(IppStatus, ippiScanInv_16s_C1, (
    const Ipp16s* pSrc,
    Ipp16s*       pDst,
    int           indxLastNonZero,
    int           scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSAD8x8_8u32s_C1R
//
//  Purpose:
//    Computes SAD.
//
//  Parameters:
//    pSrcCur      Pointer to the current block
//    srcCurStep   Width in bytes of the current plane.
//    pSrcRef      Pointer to the reference block
//    srcRefStep   Width in bytes of the reference plane.
//    mcType       Type of motion compensation
//    pDst         Pointer to the output value
//
//  Returns:
//    ippStsNoErr       No error.
//    ippStsNullPtrErr  At least one of the pointers is NULL.
*/

IPPAPI(IppStatus, ippiSAD8x8_8u32s_C1R, (
  const Ipp8u*  pSrcCur,
        int     srcCurStep,
  const Ipp8u*  pSrcRef,
        int     srcRefStep,
        Ipp32s* pDst,
        Ipp32s  mcType))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiMeanAbsDev16x16_8u32s_C1R
//
//  Purpose:
//    Computes mean absolute dev = sum(|Bij-M|).
//
//  Parameters:
//    pSrcCur   Pointer to the block 16x16.
//    srcStep   Width in bytes of the source plane.
//    pDst      Pointer to the deviation value.
//
// Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one of the pointers is NULL.
*/

IPPAPI(IppStatus, ippiMeanAbsDev16x16_8u32s_C1R, (
  const Ipp8u*  pSrc,
        int     srcStep,
        Ipp32s* pDst))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSub8x8_16s8u_C1R
//    ippiSub16x16_16s8u_C1R
//
//  Purpose:
//    Subtract two blocks and store the result in the third block
//
//  Parameters:
//    pSrc1      Pointer to the first source block.
//    src1Step   Step in bytes through the first source plane
//    pSrc2      Pointer to the second source block.
//    src2Step   Step in bytes through the second source plane
//    pSrcDst    Pointer to the destination block.
//    srcDstStep Step in bytes through the destination plane
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiSub8x8_8u16s_C1R, (
    const Ipp8u*  pSrc1,
    int           src1Step,
    const Ipp8u*  pSrc2,
    int           src2Step,
    Ipp16s*       pDst,
    int           dstStep))

IPPAPI(IppStatus, ippiSub16x16_8u16s_C1R, (
    const Ipp8u*  pSrc1,
    int           src1Step,
    const Ipp8u*  pSrc2,
    int           src2Step,
    Ipp16s*       pDst,
    int           dstStep))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSubSAD8x8_16s8u_C1R
//
//  Purpose:
//    Subtract two blocks and store the result in the third block and
//    computes a SAD
//
//  Parameters:
//    pSrc1      Pointer to the first source block.
//    src1Step   Step in bytes through the first source plane
//    pSrc2      Pointer to the second source block.
//    src2Step   Step in bytes through the second source plane
//    pDst       Pointer to the destination block.
//    dstStep    Step in bytes through the destination plane
//    pSAD       Pointer to the result of SAD
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiSubSAD8x8_8u16s_C1R, (
    const Ipp8u*  pSrc1,
    int           src1Step,
    const Ipp8u*  pSrc2,
    int           src2Step,
    Ipp16s*       pDst,
    int           dstStep,
    Ipp32s*       pSAD))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiCopy8x8_8u_C1R
//    ippiCopy16x16_8u_C1R
//
//  Purpose:
//    Copy fixed sizes blocks
//
//  Parameters:
//    pSrc      Pointer to the source block.
//    srcStep   Step in bytes through the source plane.
//    pDst      Pointer to the destination block.
//    dstStep   Step in bytes through the destination plane.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiCopy8x8_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep))

IPPAPI(IppStatus, ippiCopy16x16_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiAverage8x8_8u_C1IR
//    ippiAverage16x16_8u_C1IR
//
//  Purpose:
//    Performs averaging of two blocks with rounding.
//
//  Parameters:
//    pSrc        Pointer to the first source block.
//    srcStep     Step in bytes through the first source plane.
//    pSrcDst     Pointer to the second source/destination block.
//    srcDstStep  Step in bytes through the second plane.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiAverage8x8_8u_C1IR, (
    const Ipp8u*  pSrc,
    int           srcStep,
    Ipp8u*        pSrcDst,
    int           srcDstStep))

IPPAPI(IppStatus, ippiAverage16x16_8u_C1IR, (
    const Ipp8u*  pSrc,
    int           srcStep,
    Ipp8u*        pSrcDst,
    int           srcDstStep))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiCopy8x4HP_8u_C1R
//    ippiCopy8x8HP_8u_C1R
//    ippiCopy16x8HP_8u_C1R
//    ippiCopy16x16HP_8u_C1R
//
//  Purpose:
//    Copy fixed sizes blocks with half-pixel accuracy
//
//  Parameters:
//    pSrc      Pointer to the source block.
//    srcStep   Step in bytes through the source plane.
//    pDst      Pointer to the destination block.
//    dstStep   Step in bytes through the destination plane.
//    acc       Parameter that determines half-pixel accuracy.
//    rounding  Parameter that determines type of rounding for pixel
//              interpolation; may be 0 or 1
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiCopy8x4HP_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiCopy8x8HP_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiCopy16x8HP_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiCopy16x16HP_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiInterpolateAverage8x4_8u_C1IR
//    ippiInterpolateAverage8x8_8u_C1IR
//    ippiInterpolateAverage16x8_8u_C1IR
//    ippiInterpolateAverage16x16_8u_C1IR
//
//  Purpose:
//    Interpolate source block according to half-pixel offset and
//    average the result with destination block
//
//  Parameters:
//    pSrc       Pointer to the source block.
//    srcStep    Step in bytes through the source plane.
//    pSrcDst    Pointer to the destination block.
//    srcDstStep Step in bytes through the destination plane.
//    acc        Parameter that determines half-pixel accuracy.
//    rounding   Parameter that determines type of rounding for pixel
//               interpolation; may be 0 or 1
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiInterpolateAverage8x4_8u_C1IR, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pSrcDst,
    int          srcDstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiInterpolateAverage8x8_8u_C1IR, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiInterpolateAverage16x8_8u_C1IR, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiInterpolateAverage16x16_8u_C1IR, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiAdd8x8_16s8u_C1IRS
//
//  Purpose:
//    Add two blocks with saturation
//
//  Parameters:
//    pSrc       Pointer to the source block.
//    srcStep    Step in bytes through the source plane
//    pSrcDst    Pointer to the second source/destination block.
//    srcDstStep Step in bytes through the destination plane
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiAdd8x8_16s8u_C1IRS, (
    const Ipp16s* pSrc,
    int           srcStep,
    Ipp8u*        pSrcDst,
    int           srcDstStep))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiAdd8x8HP_16s8u_C1RS
//
//  Purpose:
//    Add interpolated with half-pixel accuracy prediction to difference with
//    saturation
//
//  Parameters:
//    pSrc1       Pointer to the 16s source block.
//    src1Step    Step in bytes through the first source plane
//    pSrc2       Pointer to the 8u source block.
//    src2Step    Step in bytes through the second source plane
//    pDst        Pointer to the destination block.
//    dstStep     Step in bytes through the destination plane
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiAdd8x8HP_16s8u_C1RS, (
    const Ipp16s* pSrc1,
    int           src1Step,
    Ipp8u*        pSrc2,
    int           src2Step,
    Ipp8u*        pDst,
    int           dstStep,
    int           acc,
    int           rounding))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiAddC8x8_16s8u_C1IR
//
//  Purpose:
//    Adds a constant to 8x8 block with saturation
//
//  Parameters:
//    value      The constant value to add to block.
//    pSrcDst    Pointer to the source/destination block.
//    srcDstStep Step in bytes through the destination block.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiAddC8x8_16s8u_C1IR, (
    Ipp16s   value,
    Ipp8u*   pSrcDst,
    int      srcDstStep))


/* ///////////////////////////////////////////////////////////////////////////
//                             H.261 Functions
//////////////////////////////////////////////////////////////////////////// */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilter8x8_H261_8u_C1R
//
//  Purpose:
//    Performs "loop" filtering on one block as specified in
//      "ITU-T Recommendation H.261", subclause 3.2.3.
//
//  Parameters:
//    pSrc             Pointer to the origin of the source block.
//    srcStep          Width in bytes of the source image plane.
//    pDst             Pointer to the origin of the destination block.
//    dstStep          Width in bytes of the destination image plane.
//
//  Returns:
//    ippStsNoErr         No error.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
*/
IPPAPI(IppStatus, ippiFilter8x8_H261_8u_C1R, (
    Ipp8u*  pSrc,
    int     srcStep,
    Ipp8u*  pDst,
    int     dstStep))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsIntra_H261_1u16s
//
//  Purpose:
//    Performs decoding and, optionally, inverse scan of quantized DCT
//    coefficients (DC and AC) for one Intra coded block. DC fixed length and
//    AC VLC decoding processes are specified in "ITU-T Recommendation H.263,
//    subclause 4.2.4.1".
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream
//                      buffer, updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by
//                      *ppBitStream. Valid within the range [0, 7],
//                      updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in
//                      the scanning order. If an error is detected while
//                      decoding a coefficient, the index of the last decoded
//                      coefficient is returned in *pIndxLastNonZero.
//                      *pIndxLastNonZero is set to -1 if there are no correctly
//                      decoded coefficients in the block.
//    scan              Type of the inverse scan, takes one of the following values:
//                      IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                      IPPVC_SCAN_NONE, indicating that no inverse scan is
//                      performed.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
*/
IPPAPI(IppStatus, ippiDecodeCoeffsIntra_H261_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsInter_H261_1u16s
//
//  Purpose:
//    Performs decoding and, optionally, inverse scan of quantized DCT
//    coefficients for one Inter coded block. Inter DCT VLC decoding process
//    is specified in "ITU-T Recommendation H.261, subclause 4.2.4.1".
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream
//                      buffer, updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by
//                      *ppBitStream. Valid within the range [0, 7],
//                      updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in
//                      the scanning order. If an error is detected while
//                      decoding a coefficient, the index of the last decoded
//                      coefficient is returned in *pIndxLastNonZero.
//                      *pIndxLastNonZero is set to -1 if there are no correctly
//                      decoded coefficients in the block.
//    scan              Type of the inverse scan, takes one of the following values:
//                      IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                      IPPVC_SCAN_NONE, indicating that no inverse scan is
//                      performed.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
*/

IPPAPI(IppStatus, ippiDecodeCoeffsInter_H261_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiReconstructCoeffsIntra_H261_1u16s
//
//  Purpose:
//    Performs decoding, dequantization and inverse scan of
//    DCT coefficients for one intra coded block.
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream buffer,
//                      updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by *ppBitStream.
//                      Valid within the range [0, 7], updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in the scanning order.
//                      If an error is detected while decoding a coefficient, the index of the
//                      last decoded coefficient is returned in *pIndxLastNonZero. *pIndxLastNonZero
//                      is set to -1 if there are no correctly decoded coefficients in the block.
//    QP                Quantization parameter.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of  the range [0, 7].
//    ippStsQPErr         QP is out of the range [1, 31].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
*/
IPPAPI(IppStatus, ippiReconstructCoeffsIntra_H261_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     QP))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiReconstructCoeffsInter_H261_1u16s
//
//  Purpose:
//    Performs decoding, dequantization and inverse scan of
//    DCT coefficients for one inter coded block.
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream buffer,
//                      updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by *ppBitStream.
//                      Valid within the range [0, 7], updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in the scanning order.
//                      If an error is detected while decoding a coefficient, the index of the
//                      last decoded coefficient is returned in *pIndxLastNonZero. *pIndxLastNonZero
//                      is set to -1 if there are no correctly decoded coefficients in the block.
//    QP                Quantization parameter.
//
//  Returns:
//    ippStsNoErr         No error.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of  the range [0, 7].
//    ippStsQPErr         QP is out of the range [1, 31].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
*/
IPPAPI(IppStatus, ippiReconstructCoeffsInter_H261_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     QP))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeCoeffsIntra_H261_16s1u
//
//  Purpose:
//    Performs encoding of quantized DCT coefficients (DC and AC) in a scan
//    order for one Intra coded block, and puts the codes into the bitstream.
//    DC fixed length and AC VLC encoding processes are specified in "ITU-T
//    Recommendation H.261, subclause 4.2.4.1".
//
//  Parameters:
//    pQCoef        Pointer to the array of quantized DCT coefficients.
//    ppBitStream   Pointer to pointer to the current byte in the bitstream
//                  buffer, updated by the function.
//    pBitOffset    Pointer  to the bit position in the byte pointed by
//                  *ppBitStream. Valid within the range [0, 7],
//                  updated by the function.
//    countNonZero  Number of non-zero coefficients in the block.
//                  Valid within the range [1, 64].
//    scan          Type of the scan to be performed on the coefficients before
//                  encoding, takes one of the following values:
//                  IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                  IPPVC_SCAN_NONE, indicating that no scan is to be
//                  performed (the input coefficients are already in the scan
//                  order).
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one of the pointers is NULL.
//    ippStsBitOffsetErr   *pBitOffset is out of  the range [0, 7].
//    ippStsOutOfRangeErr  countNonZero is out of the range [1, 64].
*/

IPPAPI(IppStatus, ippiEncodeCoeffsIntra_H261_16s1u, (
  Ipp16s* pQCoef,
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  int     countNonZero,
  int     scan))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeCoeffsInter_H261_16s1u
//
//  Purpose:
//    Performs encoding of quantized DCT coefficients in a scan order for one
//    Inter coded block, and puts the codes into the bitstream. The encoding
//    process is specified in "ITU-T Recommendation H.261, subclause 4.2.4.1".
//
//  Parameters:
//    pQCoef        Pointer to the array of quantized DCT coefficients.
//    ppBitStream   Pointer to pointer to the current byte in the bitstream
//                  buffer, updated by the function.
//    pBitOffset    Pointer  to the bit position in the byte pointed by
//                  *ppBitStream. Valid within the range [0, 7],
//                  updated by the function.
//    countNonZero  Number of non-zero coefficients in the block.
//                  Valid within the range [1, 64].
//    scan          Type of the scan to be performed on the coefficients before
//                  encoding, takes one of the following values:
//                  IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                  IPPVC_SCAN_NONE, indicating that no scan is to be
//                  performed (the input coefficients are already in the scan
//                  order).
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one of the pointers is NULL.
//    ippStsBitOffsetErr   *pBitOffset is out of  the range [0, 7].
//    ippStsOutOfRangeErr  countNonZero is out of the range [1, 64].
*/

IPPAPI(IppStatus, ippiEncodeCoeffsInter_H261_16s1u, (
  Ipp16s* pQCoef,
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  int     countNonZero,
  int     scan))


/* ///////////////////////////////////////////////////////////////////////////
//                             H.263 Functions
//////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeDCIntra_H263_1u16s
//
//  Purpose:
//    Performs fixed length decoding of the DC coefficient for one Intra coded
//    block, as specified in "ITU-T Recommendation H.263, subclause 5.4.1."
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream
//                      buffer, updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by
//                      *ppBitStream. Valid within the range [0, 7],
//                      updated by the function.
//    pDC               Pointer to the output coefficient.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr        Illegal code is detected through the stream
//                        processing.
*/

IPPAPI(IppStatus, ippiDecodeDCIntra_H263_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pDC))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsIntra_H263_1u16s
//
//  Purpose:
//    Performs decoding and, optionally, inverse scan of quantized DCT
//    coefficients for one Intra coded block. Intra AC VLC decoding process
//    is specified in "ITU-T Recommendation H.263, subclause 5.4.2", and is
//    modified as specified in the Recommendation, Annex T, clause T.4, when
//    Modified Quantization mode is in use. When in Advanced Intra Coding
//    mode, VLC Table I.2 from Annex I of the Recommendation is used for all
//    Intra DC and Intra AC coefficients, otherwise Table 16 from the
//    Recommendation is used to decode AC coefficients (starting from
//    pCoef[1]) only.
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream
//                      buffer, updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by
//                      *ppBitStream. Valid within the range [0, 7],
//                      updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in
//                      the scanning order. If an error is detected while
//                      decoding a coefficient, the index of the last decoded
//                      coefficient is returned in *pIndxLastNonZero.
//                      If there are no correctly decoded coefficients in the
//                      block, *pIndxLastNonZero is set to -1 when in Advanced
//                      Intra Coding mode, and to 0 otherwise.
//    advIntraFlag      Flag equal to a non-zero value when Advanced Intra Coding
//                      mode is in use, equal to 0 otherwise.
//    modQuantFlag      Flag equal to a non-zero value when Modified Quantization
//                      mode is in use, equal to 0 otherwise.
//    scan              Type of the inverse scan, takes one of the following values:
//                      IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                      IPPVC_SCAN_HORIZONTAL - alternate-horizontal scan,
//                      IPPVC_SCAN_VERTICAL - alternate-vertical scan,
//                      IPPVC_SCAN_NONE, indicating that no inverse scan is
//                      performed.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
*/

IPPAPI(IppStatus, ippiDecodeCoeffsIntra_H263_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     advIntraFlag,
  int     modQuantFlag,
  int     scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsInter_H263_1u16s
//
//  Purpose:
//    Performs decoding and, optionally, inverse scan of quantized DCT
//    coefficients for one Inter coded block. Inter DCT VLC decoding process
//    is specified in "ITU-T Recommendation H.263, subclause 5.4.2", and is
//    modified as specified in the Recommendation, Annex T, clause T.4, when
//    Modified Quantization mode is in use.
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream
//                      buffer, updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by
//                      *ppBitStream. Valid within the range [0, 7],
//                      updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in
//                      the scanning order. If an error is detected while
//                      decoding a coefficient, the index of the last decoded
//                      coefficient is returned in *pIndxLastNonZero.
//                      *pIndxLastNonZero is set to -1 if there are no correctly
//                      decoded coefficients in the block.
//    modQuantFlag      Flag equal to a non-zero value when Modified Quantization
//                      mode is in use, equal to 0 otherwise.
//    scan              Type of the inverse scan, takes one of the following values:
//                      IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                      IPPVC_SCAN_NONE, indicating that no inverse scan is
//                      performed.
//
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
*/

IPPAPI(IppStatus, ippiDecodeCoeffsInter_H263_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     modQuantFlag,
  int     scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInvIntra_H263_16s_C1I
//
//  Purpose:
//    This function performs inverse quantization on intra coded block.
//    When (advIntraFlag == 0 && modQuantFlag == 0), the output coefficients
//    other than pSrcDst[0] are saturated to lie in the range [-2048; 2047].
//
//  Parameters:
//    pSrcDst         Pointer to the decoded DCT coefficient of the current
//                    block
//    QP              Quantization parameter.
//    indxLastNonZero Index of the last non-zero coefficient, should be set
//                    to 63 if not known.
//    advIntraFlag    Flag equal to a non-zero value when Advanced Intra Coding
//                    mode is in use, equal to 0 otherwise.
//    modQuantFlag    Flag equal to a non-zero value when Modified Quantization
//                    mode is in use, equal to 0 otherwise.
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one input pointer is NULL
//    ippStsQPErr          QP is out of the range [1, 31].
//    ippStsOutOfRangeErr  indxLastNonZero is negative.
//
//  NOTE
//    The function can be applied to a buffer of arbitrary size (indxLastNonZero
//    can be any positive number), and can thus be used, for example, to process
//    multiple blocks in one call. (In this case for any Intra block following
//    the first one the Intra DC should be processed separately, if not in
//    Advanced Intra mode).
*/

IPPAPI(IppStatus, ippiQuantInvIntra_H263_16s_C1I, (
    Ipp16s* pSrcDst,
    int     indxLastNonZero,
    int     QP,
    int     advIntraFlag,
    int     modQuantFlag))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInvInter_H263_16s_C1I
//
//  Purpose:
//    This function performs inverse quantization on intra coded block.
//    When (advIntraFlag == 0 && modQuantFlag == 0), the output coefficients
//    other than pSrcDst[0] are saturated to lie in the range [-2048; 2047].
//
//  Parameters:
//    pSrcDst         Pointer to the decoded DCT coefficient of the current
//                    block
//    QP              Quantization parameter.
//    indxLastNonZero Index of the last non-zero coefficient, should be set
//                    to 63 if not known.
//    modQuantFlag    Flag equal to a non-zero value when Modified Quantization
//                    mode is in use, equal to 0 otherwise.
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one input pointer is NULL
//    ippStsQPErr          QP is out of the range [1, 31]
//    ippStsOutOfRangeErr  indxLastNonZero is negative.
//
//  NOTE
//    The function can be applied to a buffer of arbitrary size (indxLastNonZero
//    can be any positive number), and can thus be used, for example, to process
//    multiple blocks in one call.
//
*/

IPPAPI(IppStatus, ippiQuantInvInter_H263_16s_C1I, (
    Ipp16s* pSrcDst,
    int     indxLastNonZero,
    int     QP,
    int     modQuantFlag))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiAddBackPredPB_H263_8u_C1R
//
//  Purpose:
//    Calculates backward prediction for a B-block of a PB-frame and adds it
//    to the block, previously reconstructed with forward prediction. All the
//    operations are restricted to the bidirectionally-predicted part of the
//    B-block, the area size is defined by srcRoiSize. The backward prediction
//    is performed with pixel accuracy defined by acc, the sum of the forward
//    and backward predictions for every pixel within srcRoiSize is divided
//    by 2 (division by truncation). The bidirectional prediction procedure
//    is specified in "ITU-T Recommendation H.263, Annex G, clause G.5".
//
//  Parameters:
//    pSrc         Pointer to the origin of  the source image (P-macroblock)
//                 region of interest (ROI).
//    srcStep      Width in bytes of the source image plane.
//    srcRoiSize   Size of the source ROI.
//    pSrcDst      Pointer to the origin of  the source-destination image ROI
//                 (bidirectionally-predicted part of the block).
//    srcDstStep   Width in bytes of the source-destination image plane.
//    acc          Pixel accuracy for backward prediction: bit 0 (the least
//                 significant bit) contains the horizontal half-pixel offset,
//                 bit 1 - the vertical offset.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    The pointer is NULL.
//    ippStsSizeErr       srcRoiSize has a field with zero or negative value.
*/

IPPAPI(IppStatus, ippiAddBackPredPB_H263_8u_C1R, (
  const Ipp8u* pSrc,
  int          srcStep,
  IppiSize     srcRoiSize,
  Ipp8u*       pSrcDst,
  int          srcDstStep,
  int          acc))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiResample_H263_8u_P3R
//
//  Purpose:
//    Resamples a YCbCr picture as specified in "ITU-T Recommendation H.263,
//    Annex P". The destination picture region of interest (ROI) is mapped
//    onto the source picture ROI as defined by warpParams, the pixels falling
//    outside the source picture are treated according to fillMode.
//
//  Parameters:
//    pSrcY      Pointer to the origin of  the source image ROI in the
//               luminance plane.
//    srcYStep   Width in bytes of the source image luminance (Y) plane.
//    yRoiSize   Size of the source and destination ROI in the luminance plane.
//    pSrcCb     Pointer to the origin of the source ROI in Cb chrominance
//               plane.
//    srcCbStep  Width in bytes of the source image Cb chrominance plane.
//    pSrcCr     Pointer to the origin of the source ROI in Cr chrominance
//               plane.
//    srcCrStep  Width in bytes of the source image Cr chrominance plane.
//    pDstY      Pointer to the origin of the destination image ROI in the
//               luminance plane.
//    dstYStep   Width in bytes of the destination image luminance plane.
//    pDstCb     Pointer to the origin of  the destination ROI in Cb
//               chrominance plane.
//    dstCbStep  Width in bytes of the destination image Cb chrominance plane.
//    pDstCr     Pointer to the origin of  the destination ROI in Cr
//               chrominance plane.
//    dstCrStep  Width in bytes of the destination image Cr chrominance plane.
//    warpParams Array of warping parameters - 4 pairs of motion vectors,
//               describing, in the order they are stored in the array, how the
//               upper left, upper right, lower left, and lower right corners
//               of the destination ROI are mapped onto the source image.
//    wda        Warping displacement accuracy flag, if set to 0, pixel
//               displacements are quantized to half-pixel accuracy, otherwise -
//               to 1/16-pixel accuracy.
//    fillMode   Flag that defines the fill-mode action to be taken for the
//               values of the source pixels for which the calculated location
//               in the source image lies outside of the source image ROI.
//               Takes one of  the following values:
//               0, indicating color fill mode, the "outside" Y, Cb and Cr
//                  pixel values are set to  fillColor[0],  fillColor[1], and
//                  fillColor[2], respectively.
//               1 - black fill mode, the "outside" pixel values are set as follows:
//                  Y = 16, Cb = Cr = 128.
//               2 - gray fill mode, the "outside" pixel values are all set to 128.
//               3 - clip fill mode, the "outside" pixel values are extrapolated
//                  from the values of pixels at the ROI border, as specified in
//                  "ITU-T Recommendation H.263, Annex D".
//   fillColor   Array of fill color values used in color fill mode.
//
//  Returns:
//    ippStsNoErr        No errors.
//    ippStsNullPtrErr   At least one of the pointers is NULL.
//    ippStsSizeErr      yRoiSize has a field which is odd or less than 4.
*/

IPPAPI(IppStatus, ippiResample_H263_8u_P3R, (
  const Ipp8u*    pSrcY,
  int             srcYStep,
  IppiSize        ySrcRoiSize,
  const Ipp8u*    pSrcCb,
  int             srcCbStep,
  const Ipp8u*    pSrcCr,
  int             srcCrStep,
  Ipp8u*          pDstY,
  int             dstYStep,
  IppiSize        yDstRoiSize,
  Ipp8u*          pDstCb,
  int             dstCbStep,
  Ipp8u*          pDstCr,
  int             dstCrStep,
  IppMotionVector warpParams[4],
  int             wda,
  int             rounding,
  int             fillMode,
  int             fillColor[3]))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiUpsampleFour_H263_8u_C1R
//
//  Purpose:
//    Performs factor-of-4 picture upsampling, as specified in
//    "ITU-T Recommendation H.263, Annex P, subclause P.5.1".
//
//  Parameters:
//    pSrc                Pointer to the origin of  the source image region
//                        of interest (ROI).
//    srcStep             Width in bytes of the source image plane.
//    srcRoiSize          Size of the source ROI.
//    pDst                Pointer to the origin of  the destination image ROI.
//    dstStep             Width in bytes of the destination image plane.
//    rounding            Rounding value used in pixel interpolation,
//                        can be 0 or 1.
//    fillColor           Fill color value used for the source pixels for which
//                        the calculated location in the source image lies outside
//                        of the source image ROI. When negative, "clip" fill-mode
//                        action is employed -  the "outside" pixel values are
//                        extrapolated from the values of pixels at the ROI border,
//                        as specified in "ITU-T Recommendation H.263, Annex D".
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsSizeErr       srcRoiSize has a field which is odd or less than 4.
*/

IPPAPI(IppStatus, ippiUpsampleFour_H263_8u_C1R, (
  const Ipp8u* pSrc,
  int          srcStep,
  IppiSize     srcRoiSize,
  Ipp8u*       pDst,
  int          dstStep,
  int          rounding,
  int          fillColor))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDownsampleFour_H263_8u_C1R
//
//  Purpose:
//    Performs factor-of-4 picture downsampling, as specified in
//    "ITU-T Recommendation H.263, Annex P, subclause P.5.2".
//
//  Parameters:
//    pSrc                Pointer to the origin of  the source image region
//                        of interest (ROI).
//    srcStep             Width in bytes of the source image plane.
//    srcRoiSize          Size of the source ROI.
//    pDst                Pointer to the origin of  the destination image ROI.
//    dstStep             Width in bytes of the destination image plane.
//    rounding            Rounding value used in pixel interpolation,
//                        can be 0 or 1.
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsSizeErr       srcRoiSize has a field with zero or negative value.
*/

IPPAPI(IppStatus, ippiDownsampleFour_H263_8u_C1R, (
  const Ipp8u* pSrc,
  int          srcStep,
  IppiSize     srcRoiSize,
  Ipp8u*       pDst,
  int          dstStep,
  int          rounding))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSpatialInterpolation_H263_8u_C1R
//
//  Purpose:
//    Performs picture interpolation for 1-D or 2-D spatial scalability, as
//    specified in "ITU-T Recommendation H.263, Annex O, clause O.6".
//
//  Parameters:
//    pSrc               Pointer to the origin of  the source image region
//                       of interest (ROI).
//    srcStep            Width in bytes of the source image plane.
//    srcRoiSize         Size of the source ROI.
//    pDst               Pointer to the origin of  the destination image ROI.
//    dstStep            Width in bytes of the destination image plane.
//    interpType         Interpolation type, takes one of the following values:
//                       IPPVC_INTERP_HORIZONTAL, IPPVC_INTERP_VERTICAL, or
//                       IPPVC_INTERP_2D, indicating one-dimensional (1-D)
//                       horizontal, 1-D vertical, and 2-D interpolation,
//                       respectively.
//  Returns:
//    ippStsNoErr        No errors.
//    ippStsNullPtrErr   At least one of the pointers is NULL.
//    ippStsSizeErr      srcRoiSize has a field which is odd or less than 4.
*/

IPPAPI(IppStatus, ippiSpatialInterpolation_H263_8u_C1R, (
  const Ipp8u* pSrc,
  int          srcStep,
  IppiSize     srcRoiSize,
  Ipp8u*       pDst,
  int          dstStep,
  int          interpType))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiUpsampleFour8x8_H263_16s_C1R
//
//  Purpose:
//    Performs factor-of-4 upsampling of an 8x8 source block to a
//    16x16 destination block, as specified in
//    "ITU-T Recommendation H.263, Annex Q, clause Q.6".
//
//  Parameters:
//    pSrc      Pointer to the origin of the source 8x8 block.
//    srcStep   Width in bytes of the source image plane.
//    pDst      Pointer to the origin of the destination 16x16 block.
//    dstStep   Width in bytes of the destination image plane.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
*/

IPPAPI(IppStatus, ippiUpsampleFour8x8_H263_16s_C1R, (
  const Ipp16s* pSrc,
  int     srcStep,
  Ipp16s* pDst,
  int     dstStep))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDownsampleFour16x16_H263_16s_C1R
//
//  Purpose:
//    Performs factor-of-4 downsampling of a 16x16 source block to an
//    8x8 destination block to be applied for the block encoding in
//    Reduced-Resolution Update mode specified in "ITU-T Recommendation H.263,
//    Annex Q.
//
//  Parameters:
//    pSrc      Pointer to the origin of the source 16x16 block.
//    srcStep   Width in bytes of the source image plane.
//    pDst      Pointer to the origin of the destination 8x8 block.
//    dstStep   Width in bytes of the destination image plane.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
*/

IPPAPI(IppStatus, ippiDownsampleFour16x16_H263_16s_C1R, (
  const Ipp16s* pSrc,
  int           srcStep,
  Ipp16s*       pDst,
  int           dstStep))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblocking8x8HorEdge_H263_8u_C1IR
//    ippiFilterDeblocking8x8VerEdge_H263_8u_C1IR
//
//  Purpose:
//    Perform deblocking filtering on bordering edges, horizontal and
//    vertical respectively, of two adjacent 8x8 blocks, as specified in
//    "ITU-T Recommendation H.263, Annex J, clause J.3"
//
//  Parameters:
//    pSrcDst    Pointer to the first pixel of lower (HorEdge) or
//               right (VerEdge) block.
//    srcDstStep Width in bytes of the source and destination plane.
//    QP         Quantization parameter.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   pSrcDst pointer is NULL.
//    ippStsQPErr        QP is out of the range [1, 31].
*/

IPPAPI(IppStatus, ippiFilterDeblocking8x8HorEdge_H263_8u_C1IR, (
  Ipp8u* pSrcDst,
  int    srcDstStep,
  int    QP))

IPPAPI(IppStatus, ippiFilterDeblocking8x8VerEdge_H263_8u_C1IR, (
  Ipp8u* pSrcDst,
  int    srcDstStep,
  int    QP))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblocking16x16HorEdge_H263_8u_C1IR
//    ippiFilterDeblocking16x16VerEdge_H263_8u_C1IR
//
//  Purpose:
//    Perform deblocking filtering on bordering edges, horizontal and
//    vertical respectively, of two adjacent 16x16 blocks, as specified in
//    "ITU-T Recommendation H.263, Annex Q, subclause Q.7.2"
//
//  Parameters:
//    pSrcDst    Pointer to the first pixel of lower (HorEdge) or
//               right (VerEdge) block.
//    srcDstStep Width in bytes of the source and destination plane.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   pSrcDst pointer is NULL.
*/

IPPAPI(IppStatus, ippiFilterDeblocking16x16HorEdge_H263_8u_C1IR, (
  Ipp8u* pSrcDst,
  int    srcDstStep))

IPPAPI(IppStatus, ippiFilterDeblocking16x16VerEdge_H263_8u_C1IR, (
  Ipp8u* pSrcDst,
  int    srcDstStep))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterBlockBoundaryHorEdge_H263_8u_C1IR
//    ippiFilterBlockBoundaryVerEdge_H263_8u_C1IR
//
//  Purpose:
//    Perform block boundary filtering on bordering edges, horizontal and
//    vertical respectively, of two adjacent 16x16 blocks, as specified in
//    "ITU-T Recommendation H.263, Annex Q, subclause Q.7.1".
//
//  Parameters:
//    pSrcDst    Pointer to the origin of the lower (HorEdge) or the right
//               (VerEdge) 16x16 block.
//    srcDstStep Width in bytes of the image plane.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    The pointer is NULL.
*/

IPPAPI(IppStatus, ippiFilterBlockBoundaryHorEdge_H263_8u_C1IR, (
  Ipp8u* pSrcDst,
  int    srcDstStep))

IPPAPI(IppStatus, ippiFilterBlockBoundaryVerEdge_H263_8u_C1IR, (
  Ipp8u* pSrcDst,
  int    srcDstStep))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiReconstructCoeffsIntra_H263_1u16s
//
//  Purpose:
//    Performs decoding, dequantization and inverse scan of the DCT
//    coefficients for one intra coded block. Intra DC decoding process is
//    specified in "ITU-T Recommendation H.263, subclause 5.4.1".
//    Intra AC VLC decoding process is specified in "ITU-T Recommendation H.263,
//    subclause 5.4.2", and is modified as specified in the Recommendation,
//    Annex T, clause T.4, when Modified Quantization mode is in use. When
//    in Advanced Intra Coding mode, VLC Table I.2 from Annex I of the
//    Recommendation is used for all Intra DC and Intra AC coefficients,
//    otherwise Table 16 from the Recommendation is used to decode AC
//    coefficients (starting from pCoef[1]) only. When not in Advanced Intra
//    Coding mode, the dequantization processes for the Intra DC and for all
//    other non-zero coefficients are specified in the Recommendation,
//    subclause 6.2.1, otherwise all the coefficients are dequantized as
//    specified in the Recommendation, Annex I, clause I.3. When not in
//    Advanced Intra Coding mode and not in Modified Quantization mode, the
//    output coefficients other than the Inta DC one are clipped to the range
//    [-2048, 2047] (the Recommendation, subclause 6.2.2).
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream
//                      buffer, updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by
//                      *ppBitStream. Valid within the range [0, 7],
//                      updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in
//                      the scanning order. If an error is detected while
//                      decoding a coefficient, the index of the last decoded
//                      coefficient is returned in *pIndxLastNonZero.
//                      If there are no correctly decoded coefficients in the
//                      block, *pIndxLastNonZero is set to -1.
//    cbp               Coded block pattern, when set to 0 indicates that the
//                      block contains only Intra DC coefficient.
//    QP                Quantization parameter.
//    advIntraFlag      Flag equal to a non-zero value when Advanced Intra Coding
//                      mode is in use, equal to 0 otherwise.
//    scan              Type of the inverse scan, takes one of the following values:
//                      IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                      IPPVC_SCAN_HORIZONTAL - alternate-horizontal scan,
//                      IPPVC_SCAN_VERTICAL - alternate-vertical scan.
//    modQuantFlag      Flag equal to a non-zero value when Modified Quantization
//                      mode is in use, equal to 0 otherwise.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
//    ippStsQPErr         QP is out of the range [1, 31].
*/

IPPAPI(IppStatus, ippiReconstructCoeffsIntra_H263_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     cbp,
  int     QP,
  int     advIntraFlag,
  int     scan,
  int     modQuantFlag))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiReconstructCoeffsInter_H263_1u16s
//
//  Purpose:
//    Performs decoding, dequantization and inverse zigzag scan of the DCT
//    coefficients for one inter coded block. Inter DCT VLC decoding process
//    is specified in "ITU-T Recommendation H.263, subclause 5.4.2", and is
//    modified as specified in the Recommendation, Annex T, clause T.4, when
//    Modified Quantization mode is in use. The dequantization process is
//    specified in the Recommendation, subclause 6.2.1. When not in
//    Modified Quantization mode, the output coefficients are clipped to the
//    range [-2048, 2047] (the Recommendation, subclause 6.2.2).
//
//  Parameters:
//    ppBitStream       Pointer to pointer to the current byte in the bitstream
//                      buffer, updated by the function.
//    pBitOffset        Pointer  to the bit position in the byte pointed by
//                      *ppBitStream. Valid within the range [0, 7],
//                      updated by the function.
//    pCoef             Pointer to the output coefficients.
//    pIndxLastNonZero  Pointer to the index of the last non-zero coefficient in
//                      the scanning order. If an error is detected while
//                      decoding a coefficient, the index of the last decoded
//                      coefficient is returned in *pIndxLastNonZero.
//                      If there are no correctly decoded coefficients in the
//                      block, *pIndxLastNonZero is set to -1.
//    QP                Quantization parameter.
//    modQuantFlag      Flag equal to a non-zero value when Modified Quantization
//                      mode is in use, equal to 0 otherwise.
//
//  Returns:
//    ippStsNoErr         No errors.
//    ippStsNullPtrErr    At least one of the pointers is NULL.
//    ippStsBitOffsetErr  *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr        Illegal VL code is detected through the stream
//                        processing.
//    ippStsQPErr         QP is out of the range [1, 31].
*/

IPPAPI(IppStatus, ippiReconstructCoeffsInter_H263_1u16s, (
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     QP,
  int     modQuantFlag))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeDCIntra_H263_16s1u
//
//  Purpose:
//    Performs fixed length encoding of the DC coefficient for one Intra coded
//    block and puts the code into the bitstream. Intra DC encoding process is
//    specified in "ITU-T Recommendation H.263, subclause 5.4.1".
//
//  Parameters:
//    qDC           Quantized DC coefficient.
//    ppBitStream   Pointer to pointer to the current byte in the bitstream
//                  buffer, updated by the function.
//    pBitOffset    Pointer  to the bit position in the byte pointed by
//                  *ppBitStream. Valid within the range [0, 7],
//                  updated by the function.
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one of the pointers is NULL.
//    ippStsBitOffsetErr   *pBitOffset is out of  the range [0, 7].
*/

IPPAPI(IppStatus, ippiEncodeDCIntra_H263_16s1u, (
  Ipp16s  qDC,
  Ipp8u** ppBitStream,
  int*    pBitOffset))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeCoeffsIntra_H263_16s1u
//
//  Purpose:
//    Performs encoding of the quantized AC coefficients in a scan order for
//    one Intra coded block, and puts the codes into the bitstream. Intra AC
//    VLC encoding process is specified in "ITU-T Recommendation H.263,
//    subclause 5.4.2", and is modified as specified in the Recommendation,
//    Annex T, clause T.4, when Modified Quantization mode is in use. When in
//    Advanced Intra Coding mode, VLC Table I.2 from Annex I of the
//    Recommendation is used for all Intra DC and Intra AC coefficients,
//    otherwise Table 16 from the Recommendation is used to encode AC
//    coefficients (starting from pQCoef[1]) only.
//
//  Parameters:
//    pQCoef        Pointer to the array of quantized DCT coefficients.
//    ppBitStream   Pointer to pointer to the current byte in the bitstream
//                  buffer, updated by the function.
//    pBitOffset    Pointer  to the bit position in the byte pointed by
//                  *ppBitStream. Valid within the range [0, 7],
//                  updated by the function.
//    countNonZero  Number of non-zero coefficients in the block.
//                  Valid within the range [1, 64].
//    advIntraFlag  Flag equal to a non-zero value when Advanced Intra Coding
//                  mode is in use, equal to 0 otherwise.
//    modQuantFlag  Flag equal to a non-zero value when Modified Quantization
//                  mode is in use, equal to 0 otherwise.
//    scan          Type of the scan to be performed on the coefficients before
//                  encoding, takes one of the following values:
//                  IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                  IPPVC_SCAN_HORIZONTAL - alternate-horizontal scan,
//                  IPPVC_SCAN_VERTICAL - alternate-vertical scan,
//                  IPPVC_SCAN_NONE, indicating that no scan is to be
//                  performed (the input coefficients are already in a scan
//                  order).
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one of the pointers is NULL.
//    ippStsBitOffsetErr   *pBitOffset is out of  the range [0, 7].
//    ippStsOutOfRangeErr  countNonZero is out of the range [1, 64].
*/

IPPAPI(IppStatus, ippiEncodeCoeffsIntra_H263_16s1u, (
  Ipp16s* pQCoef,
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  int     countNonZero,
  int     advIntraFlag,
  int     modQuantFlag,
  int     scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeCoeffsInter_H263_16s1u
//
//  Purpose:
//    Performs encoding of the quantized DCT coefficients in a scan order for
//    one Inter coded block, and puts the codes into the bitstream. Inter DCT
//    VLC encoding process is specified in "ITU-T Recommendation H.263,
//    subclause 5.4.2", and is modified as specified in the Recommendation,
//    Annex T, clause T.4, when Modified Quantization mode is in use.
//
//  Parameters:
//    pQCoef        Pointer to the array of quantized DCT coefficients.
//    ppBitStream   Pointer to pointer to the current byte in the bitstream
//                  buffer, updated by the function.
//    pBitOffset    Pointer  to the bit position in the byte pointed by
//                  *ppBitStream. Valid within the range [0, 7],
//                  updated by the function.
//    countNonZero  Number of non-zero coefficients in the block.
//                  Valid within the range [1, 64].
//    modQuantFlag  Flag equal to a non-zero value when Modified Quantization
//                  mode is in use, equal to 0 otherwise.
//    scan          Type of the scan to be performed on the coefficients before
//                  encoding, takes one of the following values:
//                  IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                  IPPVC_SCAN_NONE, indicating that no scan is to be
//                  performed (the input coefficients are already in the scan
//                  order).
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one of the pointers is NULL.
//    ippStsBitOffsetErr   *pBitOffset is out of  the range [0, 7].
//    ippStsOutOfRangeErr  countNonZero is out of the range [1, 64].
*/

IPPAPI(IppStatus, ippiEncodeCoeffsInter_H263_16s1u, (
  Ipp16s* pQCoef,
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  int     countNonZero,
  int     modQuantFlag,
  int     scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantIntra_H263_16s_C1I
//
//  Purpose:
//    Performs quantization on Intra coded block according to H.263 standard.
//    The standard specifies dequantization process, while quantization
//    decision levels are not defined. When not in Advanced Intra Coding mode,
//    the Intra DC coefficient is dequantized using uniformly placed
//    reconstruction levels  with a step size of 8, and the other DCT
//    coefficients are reconstructed using equally spaced levels with a
//    central dead-zone around zero and with a step size of 2*QP
//    ("ITU-T Recommendation H.263, subclauses 4.2.4, 6.2"). When in Advanced
//    Intra Coding mode, all the block coefficients are dequantized using a
//    reconstruction spacing without a dead-zone and with a step size of 2*QP
//    ("ITU-T Recommendation H.263, Annex I, clause I.3"). When not in
//    Modified Quantization mode, the quantized Intra DC coefficient (when not
//    in Advanced Intra Coding mode) is clipped to the range [1, 254], and the
//    other quantized coefficients (all coefficients, if in Advanced Intra
//    Coding mode) are clipped to the range [-127, 127].
//
//  Parameters:
//    pSrcDst         Pointer to the decoded DCT coefficient of the current
//                    block
//    QP              Quantization parameter.
//    pCountNonZero   Pointer to the number of non-zero coefficients after
//                    quantization
//    advIntraFlag    Flag equal to a non-zero value when Advanced Intra Coding
//                    mode is in use, equal to 0 otherwise.
//    modQuantFlag    Flag equal to a non-zero value when Modified Quantization
//                    mode is in use, equal to 0 otherwise.
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one input pointer is NULL.
//    ippStsQPErr          QP is out of the range [1, 31].
//
*/

IPPAPI(IppStatus, ippiQuantIntra_H263_16s_C1I, (
   Ipp16s* pSrcDst,
   int     QP,
   int*    pCountNonZero,
   int     advIntraFlag,
   int     modQuantFlag))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInter_H263_16s_C1I
//
//  Purpose:
//    Performs quantization on Inter coded block according to H.263 standard.
//    The standard specifies dequantization process, while quantization
//    decision levels are not defined. The DCT coefficients are reconstructed
//    using equally spaced levels with a central dead-zone around zero and
//    with a  step size of 2*QP ("ITU-T Recommendation H.263, subclauses 4.2.4,
//    6.2"). When not in Modified Quantization mode, the quantized coefficients
//    are clipped to the range [-127, 127].
//
//  Parameters:
//    pSrcDst         Pointer to the decoded DCT coefficient of the current
//                    block
//    QP              Quantization parameter.
//    pCountNonZero   Pointer to the number of non-zero coefficients after
//                    quantization
//    modQuantFlag    Flag equal to a non-zero value when Modified Quantization
//                    mode is in use, equal to 0 otherwise.
//
//  Returns:
//    ippStsNoErr          No error.
//    ippStsNullPtrErr     At least one input pointer is NULL.
//    ippStsQPErr          QP is out of the range [1, 31].
//
*/

IPPAPI(IppStatus, ippiQuantInter_H263_16s_C1I, (
   Ipp16s* pSrcDst,
   int     QP,
   int*    pCountNonZero,
   int     modQuantFlag))



/* ///////////////////////////////////////////////////////////////////////////
//                              MPEG-4 Functions
//////////////////////////////////////////////////////////////////////////// */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblocking8x8HorEdge_MPEG4_8u_C1IR
//
//  Purpose:
//    Performs deblocking filtering on a horizontal edge of two adjacent
//    blocks in the reconstructed frame, which is described in Annex F.3.1.
//
//  Parameters:
//    pSrcDst      Pointer to the first pixel of the lower block.
//    step         Width of the source and destination plane.
//    QP           Quantization parameter.
//    THR1, THR2   Threshold values specifying the filter mode
//
//  Returns:
//    ippStsNoErr         No error.
//    ippStsNullPtrErr    One of the specified pointers is NULL.
//    ippStsQPErr         QP is out of range [1, 31].
*/

IPPAPI(IppStatus, ippiFilterDeblocking8x8HorEdge_MPEG4_8u_C1IR, (
  Ipp8u*  pSrcDst,
  int     step,
  int     QP,
  int     THR1,
  int     THR2))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblocking8x8VerEdge_MPEG4_8u_C1IR
//
//  Purpose:
//    Performs deblocking filtering on a vertical edge of two adjacent
//    blocks in the reconstructed frame, which is described in Annex F.3.1.
//
//  Parameters:
//    pSrcDst    Pointer to the first pixel of the right block.
//    step       Width of the source and destination plane.
//    QP         Quantization parameter.
//    THR1,THR2  Threshold values specifying the filter mode
//
//  Returns:
//    ippStsNoErr         No error.
//    ippStsNullPtrErr    One of the specified pointers is NULL.
//    ippStsQPErr         QP is out of range [1, 31].
*/

IPPAPI(IppStatus, ippiFilterDeblocking8x8VerEdge_MPEG4_8u_C1IR, (
  Ipp8u*  pSrcDst,
  int     step,
  int     QP,
  int     THR1,
  int     THR2))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeringingThreshold_MPEG4_8u_P3R
//
//  Purpose:
//    Computes threshold values of 6 blocks in one macroblock
//    (4Y, Cb, Cr) for the deringing filter described in Annex F.3.2.1.
//
//  Parameters:
//    pSrcY          Pointer to the first pixel of the first Y block
//                   in the current macroblock.
//    stepY          Width of the Y plane.
//    pSrcCb         Pointer to the first pixel of the Cb block
//    stepCb         Width of the Cb plane.
//    pSrcCr         Pointer to the first pixel of the Cr block
//    stepCr         Width of the Cr plane.
//    threshold      Array of 6 threshold values for each block
//
//  Returns:
//    ippStsNoErr      No error.
//    ippStsNullPtrErr One of the specified pointers is NULL.
*/

IPPAPI(IppStatus, ippiFilterDeringingThreshold_MPEG4_8u_P3R, (
  const Ipp8u*  pSrcY,
        int     stepY,
  const Ipp8u*  pSrcCb,
        int     stepCb,
  const Ipp8u*  pSrcCr,
        int     stepCr,
        int     threshold[6]))


/* ///////////////////////////////////////////////////////////////////////////
// Name:
//   ippiFilterDeringingSmooth8x8_MPEG4_8u_C1R
//
//  Purpose:
//    Performs index acquisition and adaptive smoothing
//    (Annex F.3.2) of a block
//
//  Parameters:
//    pSrc         Pointer to the first pixel of the source block
//    srcStep      Width of the source plane.
//    pDst         Pointer to the first pixel of the destination block
//    dstStep      Width of the destination plane.
//    QP           Quantization parameter.
//    threshold    Threshold values for block
//
//  Returns:
//    ippStsNoErr       No error.
//    ippStsNullPtrErr  One of the specified pointers is NULL.
//    ippStsQPErr       QP is out of range [1, 31].
*/

IPPAPI(IppStatus, ippiFilterDeringingSmooth8x8_MPEG4_8u_C1R, (
  const Ipp8u*  pSrc,
        int     srcStep,
        Ipp8u*  pDst,
        int     dstStep,
        int     QP,
        int     threshold))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeDCIntra_MPEG4_16s1u
//
//  Purpose:
//    Encodes one DC coefficient for intra coded block.
//
//  Parameters:
//    dcCoeff       DC coefficient to be encoded
//    ppBitStream   Pointer to the pointer to the current byte in
//                  the bitstream, it is updated after encoding.
//    pBitOffset    Pointer to the bit position in the byte pointed by
//                  *ppBitStream, it is updated after encoding.
//    blockType     Indicates the type of block, takes one of the following
//                  values:
//                      IPPVC_BLOCK_LUMA - for luma and alpha blocks,
//                      IPPVC_BLOCK_CHROMA - for chroma blocks
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
*/

IPPAPI(IppStatus, ippiEncodeDCIntra_MPEG4_16s1u, (
    Ipp16s   dcCoeff,
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    int      blockType))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeCoeffsIntra_MPEG4_16s1u
//
//  Purpose:
//    Encodes DCT coefficients for intra coded block.
//
//  Parameters:
//    pCoeffs          Pointer to the DCT coefficients
//    ppBitStream      Pointer to the pointer to the current byte in
//                     the bitstream, it is updated after encoding.
//    pBitOffset       Pointer to the bit position in the byte pointed by
//                     *ppBitStream, it is updated after encoding.
//    countNonZero     The number of nonzero coefficients
//    rvlcFlag         This is a flag which when set to '0' indicates that VLC
//                     tables B.16, B.18, B.19 and B.21 [ISO14496] will be used
//                     when decoding DCT coefficients otherwise the RVLC tables
//                     B.23, B.24 and B.25 [ISO14496] will be used.
//    noDCFlag         This is a flag which when set to '0' indicates that
//                     pCoeffs will be encoded starting with zero element otherwise
//                     with first
//    scan             Type of the scan, takes one of the following values:
//                       IPPVC_SCAN_NONE, indicating do not perform scan,
//                       IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                       IPPVC_SCAN_HORIZONTAL - alternate-horizontal scan,
//                       IPPVC_SCAN_VERTICAL - alternate-vertical scan
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
*/

IPPAPI(IppStatus, ippiEncodeCoeffsIntra_MPEG4_16s1u, (
    const Ipp16s*  pCoeffs,
    Ipp8u**        ppBitStream,
    int*           pBitOffset,
    int            countNonZero,
    int            rvlcFlag,
    int            noDCFlag,
    int            scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeCoeffsInter_MPEG4_16s1u
//
//  Purpose:
//    Encodes DCT coefficients for inter coded block..
//
//  Parameters:
//    pCoeffs          Pointer to the DCT coefficients
//    ppBitStream      Pointer to the pointer to the current byte in
//                     the bitstream, it is updated after block decoding.
//    pBitOffset       Pointer to the bit position in the byte pointed by
//                     *ppBitStream, it is updated after block decoding.
//    pCoeffs          Pointer to the decoded DCT coefficient of the current
//                     block
//    countNonZero     The number of nonzero coefficients
//    rvlcFlag         This is a flag which when set to '0' indicates that VLC
//                     tables B.16, B.18, B.19 and B.21 [ISO14496] will be used
//                     when decoding DCT coefficients otherwise the RVLC tables
//                     B.23, B.24 and B.25 [ISO14496] will be used.
//    scan             Type of the scan, takes one of the following values:
//                       IPPVC_SCAN_NONE, indicating do not perform scan,
//                       IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                       IPPVC_SCAN_VERTICAL - alternate-vertical scan
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
*/

IPPAPI(IppStatus, ippiEncodeCoeffsInter_MPEG4_16s1u, (
    const Ipp16s*  pCoeffs,
    Ipp8u**        ppBitStream,
    int*           pBitOffset,
    int            countNonZero,
    int            rvlcFlag,
    int            scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeDCIntra_MPEG4_1u16s
//
//  Purpose:
//    Performs VLC decoding of the DC coefficient for one intra coded block
//    using Intra DC VLC.
//
//  Parameters:
//    ppBitStream   Pointer to the pointer to the current byte in
//                  the bitstream, it is updated after block decoding.
//    pBitOffset    Pointer to the bit position in the byte pointed by
//                  *ppBitStream, it is updated after block decoding.
//    pDst          Pointer to the decoded DC coefficient of the current block
//    blockType     Indicates the type of block, takes one of the following
//                  values:
//                      IPPVC_BLOCK_LUMA - for luma and alpha blocks,
//                      IPPVC_BLOCK_CHROMA - for chroma blocks
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
//    ippStsVLCCodeErr   An illegal code is detected through the
//                       DC stream processing.
*/

IPPAPI(IppStatus, ippiDecodeDCIntra_MPEG4_1u16s, (
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    Ipp16s*  pDst,
    int      blockType))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsIntra_MPEG4_1u16s
//
//  Purpose:
//    Performs VLC decoding of the DCT coefficient for one intra coded block
//    using Intra DC VLC.
//
//  Parameters:
//    ppBitStream      Pointer to the pointer to the current byte in
//                     the bitstream, it is updated after block decoding.
//    pBitOffset       Pointer to the bit position in the byte pointed by
//                     *ppBitStream, it is updated after block decoding.
//    pCoeffs          Pointer to the decoded DCT coefficient of the current
//                     block
//    pIndxLastNonZero Pointer to the index of last non zero coefficient.
//                     In case of error during decoding the index on wich
//                     error occurred will be stored in pIndxLastNonZero
//    rvlcFlag         This is a flag which when set to '0' indicates that VLC
//                     tables B.16, B.18, B.19 and B.21 [ISO14496] will be used
//                     when decoding DCT coefficients otherwise the RVLC tables
//                     B.23, B.24 and B.25 [ISO14496] will be used.
//    noDCFlag         This is a flag which when set to '0' indicates that
//                     pCoeffs will be set starting with zero element otherwise
//                     with first
//    scan             Type of the scan, takes one of the following values:
//                       IPPVC_SCAN_NONE, indicating do not perform inverse scan,
//                       IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                       IPPVC_SCAN_HORIZONTAL - alternate-horizontal scan,
//                       IPPVC_SCAN_VERTICAL - alternate-vertical scan
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
//    ippStsVLCCodeErr   An illegal code is detected through the
//                       DC stream processing.
*/

IPPAPI(IppStatus, ippiDecodeCoeffsIntra_MPEG4_1u16s, (
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    Ipp16s*  pCoeffs,
    int*     pIndxLastNonZero,
    int      rvlcFlag,
    int      noDCFlag,
    int      scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsIntraRVLCBack_MPEG4_1u16s
//
//  Purpose:
//    Decodes DCT coefficients in backward direction for intra coded block
//    using RVLC.
//
//  Parameters:
//    ppBitStream      Pointer to the pointer to the current byte in
//                     the bitstream, it is updated after block decoding.
//    pBitOffset       Pointer to the bit position in the byte pointed by
//                     *ppBitStream, it is updated after block decoding.
//    pCoeffs          Pointer to the decoded DCT coefficient of the current
//                     block
//    pIndxLastNonZero Pointer to the index of last non zero coefficient.
//                     In case of error during decoding the index on wich
//                     error occurred will be stored in pIndxLastNonZero
//    noDCFlag         This is a flag which when set to '0' indicates that
//                     pCoeffs will be set starting with zero element otherwise
//                     with first
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
//    ippStsVLCCodeErr   An illegal code is detected through the
//                       DC stream processing.
*/

IPPAPI(IppStatus, ippiDecodeCoeffsIntraRVLCBack_MPEG4_1u16s, (
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    Ipp16s*  pCoeffs,
    int*     pIndxLastNonZero,
    int      noDCFlag))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsInter_MPEG4_1u16s
//
//  Purpose:
//    Performs VLC decoding of the DCT coefficient for one inter coded block
//    using Inter DC VLC.
//
//  Parameters:
//    ppBitStream      Pointer to the pointer to the current byte in
//                     the bitstream, it is updated after block decoding.
//    pBitOffset       Pointer to the bit position in the byte pointed by
//                     *ppBitStream, it is updated after block decoding.
//    pCoeffs          Pointer to the decoded DCT coefficient of the current
//                     block
//    pIndxLastNonZero Pointer to the index of last non zero coefficient.
//                     In case of error during decoding the index on wich
//                     error occurred will be stored in pIndxLastNonZero
//    rvlcFlag         This is a flag which when set to '0' indicates that VLC
//                     tables B.16, B.18, B.19 and B.21 [ISO14496] will be used
//                     when decoding DCT coefficients otherwise the RVLC tables
//                     B.23, B.24 and B.25 [ISO14496] will be used.
//    scan             Type of the scan, takes one of the following values:
//                       IPPVC_SCAN_NONE, indicating do not perform inverse scan,
//                       IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                       IPPVC_SCAN_VERTICAL - alternate-vertical scan
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
//    ippStsVLCCodeErr   An illegal code is detected through the
//                       DC stream processing.
*/

IPPAPI(IppStatus, ippiDecodeCoeffsInter_MPEG4_1u16s, (
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    Ipp16s*  pCoeffs,
    int*     pIndxLastNonZero,
    int      rvlcFlag,
    int      scan))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCoeffsInterRVLCBack_MPEG4_1u16s
//
//  Purpose:
//    Decodes DCT coefficients in backward direction for inter coded block
//    using RVLC.
//
//  Parameters:
//    ppBitStream      Pointer to the pointer to the current byte in
//                     the bitstream, it is updated after block decoding.
//    pBitOffset       Pointer to the bit position in the byte pointed by
//                     *ppBitStream, it is updated after block decoding.
//    pCoeffs          Pointer to the decoded DCT coefficient of the current
//                     block
//    pIndxLastNonZero Pointer to the index of last non zero coefficient.
//                     In case of error during decoding the index on wich
//                     error occurred will be stored in pIndxLastNonZero
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
//    ippStsVLCCodeErr   An illegal code is detected through the
//                       DC stream processing.
*/

IPPAPI(IppStatus, ippiDecodeCoeffsInterRVLCBack_MPEG4_1u16s, (
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    Ipp16s*  pCoeffs,
    int*     pIndxLastNonZero))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiReconstructCoeffsInter_MPEG4_1u16s
//
//  Purpose:
//    Performs VLC decoding of the DCT coefficient for one inter coded block
//    using Inter DC VLC.
//
//  Parameters:
//    ppBitStream      Pointer to the pointer to the current byte in
//                     the bitstream, it is updated after block decoding.
//    pBitOffset       Pointer to the bit position in the byte pointed by
//                     *ppBitStream, it is updated after block decoding.
//    pCoeffs          Pointer to the decoded DCT coefficient of the current
//                     block
//    pIndxLastNonZero Pointer to the index of last non zero coefficient.
//                     In case of error during decoding the index on wich
//                     error occurred will be stored in pIndxLastNonZero
//    rvlcFlag         This is a flag which when set to '0' indicates that VLC
//                     tables B.16, B.18, B.19 and B.21 [ISO14496] will be used
//                     when decoding DCT coefficients otherwise the RVLC tables
//                     B.23, B.24 and B.25 [ISO14496] will be used.
//    scan             Type of the scan, takes one of the following values:
//                       IPPVC_SCAN_ZIGZAG, indicating the classical zigzag scan,
//                       IPPVC_SCAN_VERTICAL - alternate-vertical scan
//    pQuantInvInterSpec Pointer to the structure IppiQuantInterSpec_16s which
//    QP               Quantization parameter.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   One of the specified pointers is NULL.
//    ippStsBitOffsetErr *pBitOffset is out of the range [0, 7].
//    ippStsVLCErr       An illegal code is detected through the
//                       stream processing.
//    ippStsQPErr        Indicates an error condition if QP is out of the
//                       range [1; 2^(bitsPerPixel - 3) - 1]
*/

IPPAPI(IppStatus, ippiReconstructCoeffsInter_MPEG4_1u16s, (
    Ipp8u**                            ppBitStream,
    int*                               pBitOffset,
    Ipp16s*                            pCoeffs,
    int*                               pIndxLastNonZero,
    int                                rvlcFlag,
    int                                scan,
    const IppiQuantInvInterSpec_MPEG4* pQuantInvInterSpec,
    int                                QP))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInvIntraInit_MPEG4
//    ippiQuantInvInterInit_MPEG4
//
//  Purpose:
//    Initialize a IppiQuantInvIntraSpec_16s or IppiQuantInvInterSpec_MPEG4 for
//    future usage in ippiQuantInvIntra_MPEG4_16s_C1I or
//    ippiQuantInvInter_MPEG4_16s_C1I. If pQuantMatrix is NULL, the second
//    quantization method will be used; otherwise, the first
//
//  Parameters:
//    pQuantMatrix  Pointer to the quantization matrix size of 64.
//    pSpec         Pointer to the structure IppiQuantInvIntraSpec_16s or
//                  IppiQuantInvInterSpec_MPEG4 which will initialized.
//    bitsPerPixel  Video data precision used for saturation of result. This
//                  parameter is valid within the range [4; 12].
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when pointer pSpec is NULL.
//    ippStsOutOfRangeErrIndicates an error when bitsPerPixel is out of
//                       the range [4; 12].
*/

IPPAPI(IppStatus, ippiQuantInvIntraInit_MPEG4, (
    const Ipp8u*                 pQuantMatrix,
    IppiQuantInvIntraSpec_MPEG4* pSpec,
    int                          bitsPerPixel))

IPPAPI(IppStatus, ippiQuantInvInterInit_MPEG4, (
    const Ipp8u*                 pQuantMatrix,
    IppiQuantInvInterSpec_MPEG4* pSpec,
    int                          bitsPerPixel))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInvIntraGetSize_MPEG4
//    ippiQuantInvInterGetSize_MPEG4
//
//  Purpose:
//    Return size of IppiQuantInvIntraSpec_MPEG4 or IppiQuantInvInterSpec_MPEG4.
//
//  Parameters:
//    pSpecSize Pointer to the resulting size of the structure
//    IppiQuantInvIntraSpec_MPEG4 or IppiQuantInvInterSpec_MPEG4.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when pointer pSpecSize is NULL.
*/

IPPAPI(IppStatus, ippiQuantInvIntraGetSize_MPEG4, (
    int* pSpecSize))

IPPAPI(IppStatus, ippiQuantInvInterGetSize_MPEG4, (
    int* pSpecSize))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantInvIntra_MPEG4_16s_C1I
//    ippiQuantInvInter_MPEG4_16s_C1I
//
//  Purpose:
//    Perform inverse quantization. Output coefficients are saturated to lie
//    in the range: [-2^(bitsPerPixel + 3); 2^(bitsPerPixel + 3) - 1]. If the
//    index of last nonzero coefficient is known the parameter len may be set
//    to indxLastNonZero + 1 otherwise the len should be set to 64
//
//  Parameters:
//    pCoeffs       Pointer to the decoded DCT coefficient of the current
//                  block
//    indxLastNonZero The index of last non zero coeff..
//    pSpec         Pointer to the structure IppiQuantInvIntraSpec_MPEG4 or
//                  IppiQuantInvInterSpec_MPEG4 which will initialized.
//    QP            Quantization parameter.
//    blockType     Indicates the type of block, takes one of the following
//                  values:
//                     IPPVC_BLOCK_LUMA - for luma and alpha blocks,
//                     IPPVC_BLOCK_CHROMA - for chroma blocks
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
//    ippStsQPErr        Indicates an error condition if QP is out of the
//                       range [1; 2^(bitsPerPixel - 3) - 1]
*/

IPPAPI(IppStatus, ippiQuantInvIntra_MPEG4_16s_C1I, (
    Ipp16s*                            pCoeffs,
    int                                indxLastNonZero,
    const IppiQuantInvIntraSpec_MPEG4* pSpec,
    int                                QP,
    int                                blockType))

IPPAPI(IppStatus, ippiQuantInvInter_MPEG4_16s_C1I, (
    Ipp16s*                            pCoeffs,
    int                                indxLastNonZero,
    const IppiQuantInvInterSpec_MPEG4* pSpec,
    int                                QP))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantIntraInit_MPEG4
//    ippiQuantInterInit_MPEG4
//
//  Purpose:
//    Initialize a IppiQuantIntraSpec_MPEG4 or IppiQuantInterSpec_MPEG4 for
//    future usage in ippiQuantIntra_MPEG4_16s_C1I or
//    ippiQuantInter_MPEG4_16s_C1I. If pQuantMatrix is NULL, the second
//    quantization method will be used; otherwise, the first
//
//  Parameters:
//    pQuantMatrix  Pointer to the quantization matrix size of 64.
//    pSpec         Pointer to the structure IppiQuantIntraSpec_MPEG4 or
//                  IppiQuantInterSpec_MPEG4 which will initialized.
//    bitsPerPixel  Video data precision used for saturation of result. This
//                  parameter is valid within the range [4; 12].
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when pointer pSpec is NULL.
//    ippStsOutOfRangeErrIndicates an error when bitsPerPixel is out of
//                       the range [4; 12].
*/

IPPAPI(IppStatus, ippiQuantIntraInit_MPEG4, (
    const Ipp8u*              pQuantMatrix,
    IppiQuantIntraSpec_MPEG4* pSpec,
    int                       bitsPerPixel))

IPPAPI(IppStatus, ippiQuantInterInit_MPEG4, (
    const Ipp8u*              pQuantMatrix,
    IppiQuantInterSpec_MPEG4* pSpec,
    int                       bitsPerPixel))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantIntraGetSize_MPEG4
//    ippiQuantInterGetSize_MPEG4
//
//  Purpose:
//    Return size of IppiQuantIntraSpec_MPEG4 or IppiQuantInterSpec_MPEG4.
//
//  Parameters:
//    pSpecSize Pointer to the resulting size of the structure
//    IppiQuantIntraSpec_MPEG4 or IppiQuantInterSpec_MPEG4.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when pointer pSpecSize is NULL.
*/

IPPAPI(IppStatus, ippiQuantIntraGetSize_MPEG4, (
    int* pSpecSize))

IPPAPI(IppStatus, ippiQuantInterGetSize_MPEG4, (
    int* pSpecSize))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantIntra_MPEG4_16s_C1I
//    ippiQuantInter_MPEG4_16s_C1I
//
//  Purpose:
//    Perform quantization. Output coefficients are saturated to lie in the
//    range: [-2047; 2047]. Also these functions calculate the number of
//    nonzero coefficients after quantization
//
//  Parameters:
//    pCoeffs       Pointer to the decoded DCT coefficient of the current
//                  block
//    pSpec         Pointer to the structure IppiQuantIntraSpec_MPEG4 or
//                  IppiQuantInterSpec_MPEG4 which will initialized.
//    QP            Quantization parameter.
//    pCountNonZero Pointer to the count of non zero coefficients.
//    blockType     Indicates the type of block, takes one of the following
//                  values:
//                     IPPVC_BLOCK_LUMA - for luma and alpha blocks,
//                     IPPVC_BLOCK_CHROMA - for chroma blocks
//    len           The number of coefficients to dequantize.
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
//    ippStsQPErr        Indicates an error condition if QP is out of the
//                       range [1; 2^(bitsPerPixel - 3) - 1]
*/

IPPAPI(IppStatus, ippiQuantIntra_MPEG4_16s_C1I, (
    Ipp16s*                         pCoeffs,
    const IppiQuantIntraSpec_MPEG4* pSpec,
    int                             QP,
    int*                            pCountNonZero,
    int                             blockType))

IPPAPI(IppStatus, ippiQuantInter_MPEG4_16s_C1I, (
    Ipp16s*                         pCoeffs,
    const IppiQuantInterSpec_MPEG4* pSpec,
    int                             QP,
    int*                            pCountNonZero))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiWarpInit_MPEG4
//
//  Purpose:
//    Init an IppiWarpSpec_MPEG4 structure for further usage in GMC or Sprite
//    reconstruction
//
//  Parameters:
//    pSpec             Pointer to the IppiWarpSpec_MPEG4 structure.
//    pDU               Pointer to array of the x-coordinate of warping points
//    pDV               Pointer to array of the y-coordinate of warping points
//    numWarpingPoints  The number of warping points, valid in [0-4].
//    spriteType        Indicates a sprite coding mode,
//                        IPPVC_SPRITE_STATIC - static sprites
//                        IPPVC_SPRITE_GMC   - GMC(Global Motion Compensation)
//    warpingAccuracy   The accuracy of warping, valid in [0-3].
//    roundingType      Parameter that determines type of rounding for pixel
//                      approximation; may be 0 or 1
//    quarterSample     Parameter that indicates a quarter sample mode;
//                      may be 0 or 1.
//    fcode             Parameter that determines the range of motion vector,
//                      valid in diapason [1-7]
//    spriteRect        Parameter that determines rectangle region for Sprite
//                      (or ref VOP for GMC)
//    vopRect           Parameter that determines rectangle region for
//                      current VOP
//
//  Returns:
//    ippStsNoErr         No error.
//    ippStsNullPtrErr    Indicates an error when pSpec is NULL or pDU,pDV are
//                        NULL for numWarpingPoints > 0.
//    ippStsSizeErr       Indicates an error when width or height of images is
//                        less than or equal to zero
//    ippStsOutOfRangeErr Indicates an error when numWarpingPoints or
//                        warpingAccuracy are out of valid diapason.
*/

IPPAPI(IppStatus, ippiWarpInit_MPEG4, (
    IppiWarpSpec_MPEG4* pSpec,
    const int*          pDU,
    const int*          pDV,
    int                 numWarpingPoints,
    int                 spriteType,
    int                 warpingAccuracy,
    int                 roundingType,
    int                 quarterSample,
    int                 fcode,
    const IppiRect*     spriteRect,
    const IppiRect*     vopRect))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiWarpGetSize_MPEG4
//
//  Purpose:
//    Return size of IppiWarpSpec_MPEG4 structure
//
//  Parameters:
//    pSpecSize  Pointer to the resulting size of the structure
//               IppiWarpSpec_MPEG4.
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when pointer pSpecSize is NULL.
*/

IPPAPI(IppStatus, ippiWarpGetSize_MPEG4, (
    int*  pSpecSize))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiWarpLuma_MPEG4_8u_C1R
//
//  Purpose:
//    Warp an arbitrary luma rectangular region according motion parameters
//    stored in IppiWarpSpec_MPEG4 structure
//
//  Parameters:
//    pSrcY     Pointer to the origin of the source plane.
//    srcStep   Step in bytes through the source plane
//    pDst      Pointer to the destination region.
//    dstStep   Step in bytes through the destination plane
//    dstRect   The rectangular destination region
//    pSpec     Pointer to the structure with motion parameters.
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when at least one pointer is NULL.
*/

IPPAPI(IppStatus, ippiWarpLuma_MPEG4_8u_C1R, (
    const Ipp8u*              pSrcY,
    int                       srcStepY,
    Ipp8u*                    pDstY,
    int                       dstStepY,
    const IppiRect*           dstRect,
    const IppiWarpSpec_MPEG4* pSpec))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiWarpChroma_MPEG4_8u_P2R
//
//  Purpose:
//    Warp an arbitrary chroma rectangular region according motion parameters
//    stored in IppiWarpSpec_MPEG4 structure
//
//  Parameters:
//    pSrcCb    Pointer to the origin of the first source plane.
//    srcStepCb Step in bytes through the first source plane
//    pSrcCr    Pointer to the origin of the second source plane.
//    srcStepCr Step in bytes through the second source plane
//    pDstCb    Pointer to the first destination plane.
//    dstStepCb Step in bytes through the first destination plane
//    pDstCr    Pointer to the second destination plane.
//    dstStepCr Step in bytes through the second destination plane
//    dstRect   The rectangular destination region
//    pSpec     Pointer to the structure with motion parameters.
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when at least one pointer is NULL.
*/

IPPAPI(IppStatus, ippiWarpChroma_MPEG4_8u_P2R, (
    const Ipp8u*              pSrcCb,
    int                       srcStepCb,
    const Ipp8u*              pSrcCr,
    int                       srcStepCr,
    Ipp8u*                    pDstCb,
    int                       dstStepCb,
    Ipp8u*                    pDstCr,
    int                       dstStepCr,
    const IppiRect*           dstRect,
    const IppiWarpSpec_MPEG4* pSpec))




/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiCalcGlobalMV_MPEG4
//
//  Purpose:
//    Calculate a Global Motion Vector for one macroblock according motion
//    parameters stored in IppiWarpSpec_MPEG4 structure
//
//  Parameters:
//    xOffset   The left coordinate of top-left corner of luma 16x16 block
//    yOffset   The top coordinate of top-left corner of luma 16x16 block
//    pGMV      Pointer to the resulting motion vector.
//    pSpec     Pointer to the structure with motion parameters.
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when at least one pointer is NULL.
*/

IPPAPI(IppStatus, ippiCalcGlobalMV_MPEG4, (
    int                        xOffset,
    int                        yOffset,
    IppMotionVector*           pGMV,
    const IppiWarpSpec_MPEG4*  pSpec))




/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiChangeSpriteBrightness_MPEG4_8u_C1IR
//
//  Purpose:
//    Change brightness after sprite warping
//
//  Parameters:
//    pSrcDst      Pointer to the video plane.
//    srcDstStep   Step in bytes through the video plane
//    width        The width of the video plane
//    height       The height of the video plane
//    brightnessChangeFactor Factor for changing brightness; valid in diapason
//                  [-112; 1648]
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   Indicates an error when at least one pointer is NULL.
//    ippStsOutOfRangeErrIndicates an error when brightnessChangeFactor is out
//                       of valid diapason.
*/

IPPAPI(IppStatus, ippiChangeSpriteBrightness_MPEG4_8u_C1IR, (
    Ipp8u*  pSrcDst,
    int     srcDstStep,
    int     width,
    int     height,
    int     brightnessChangeFactor))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiCopy8x8QP_MPEG4_8u_C1R
//    ippiCopy16x8QP_MPEG4_8u_C1R
//    ippiCopy16x16QP_MPEG4_8u_C1R
//
//  Purpose:
//    Copy fixed sizes blocks with quarter-pixel accuracy
//
//  Parameters:
//    pSrc      Pointer to the source block.
//    srcStep   Step in bytes through the source plane.
//    pDst      Pointer to the destination block.
//    dstStep   Step in bytes through the destination plane.
//    acc       Parameter that determines quarter-pixel accuracy.
//    rounding  Parameter that determines type of rounding for pixel
//              interpolation; may be 0 or 1
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
*/

IPPAPI(IppStatus, ippiCopy8x8QP_MPEG4_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiCopy16x8QP_MPEG4_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))

IPPAPI(IppStatus, ippiCopy16x16QP_MPEG4_8u_C1R, (
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiOBMC8x8HP_MPEG4_8u_C1R
//    ippiOBMC16x16HP_MPEG4_8u_C1R
//    ippiOBMC8x8QP_MPEG4_8u_C1R
//
//  Purpose:
//    Performs the OBMC for a block with half-pixel and quarter pixel accuracy
//
//  Parameters:
//    pSrc         Pointer to the first pixel of the reference macroblock.
//    srcStep      Width of the source plane.
//    pDst         Pointer to the first pixel of the destination macroblock.
//    dstStep      Width of the destination plane.
//    pMVCur       Pointer to the motion vector for the current block.
//    pMVLeft      Pointer to the motion vector for left block.
//    pMVRight     Pointer to the motion vector for right block.
//    pMVAbove     Pointer to the motion vector for above block.
//    pMVBelow     Pointer to the motion vector for bellow block.
//    rounding     Parameter specifying type of rounding according to 7.6.2.1
//
//  Returns:
//    ippStsNoErr No error.
//    ippStsNullPtrErr  One of the specified pointers is NULL.
*/

IPPAPI(IppStatus, ippiOBMC8x8HP_MPEG4_8u_C1R, (
  const Ipp8u*              pSrc,
        int                 srcStep,
        Ipp8u*              pDst,
        int                 dstStep,
  const IppMotionVector*    pMVCur,
  const IppMotionVector*    pMVLeft,
  const IppMotionVector*    pMVRight,
  const IppMotionVector*    pMVAbove,
  const IppMotionVector*    pMVBelow,
        int                 rounding))

IPPAPI(IppStatus, ippiOBMC16x16HP_MPEG4_8u_C1R, (
  const Ipp8u*              pSrc,
        int                 srcStep,
        Ipp8u*              pDst,
        int                 dstStep,
  const IppMotionVector*    pMVCur,
  const IppMotionVector*    pMVLeft,
  const IppMotionVector*    pMVRight,
  const IppMotionVector*    pMVAbove,
  const IppMotionVector*    pMVBelow,
        int                 rounding))

IPPAPI(IppStatus, ippiOBMC8x8QP_MPEG4_8u_C1R, (
  const Ipp8u*              pSrc,
        int                 srcStep,
        Ipp8u*              pDst,
        int                 dstStep,
  const IppMotionVector*    pMVCur,
  const IppMotionVector*    pMVLeft,
  const IppMotionVector*    pMVRight,
  const IppMotionVector*    pMVAbove,
  const IppMotionVector*    pMVBelow,
        int                 rounding))


/* ///////////////////////////////////////////////////////////////////////////
//                     H.264 Video Decoder Functions
//////////////////////////////////////////////////////////////////////////// */

/*////////////////////////////////////////////////////////////////////////////
//  Name:
//   ippiPredictIntra_4x4_H264_8u_C1IR
//
//  Purpose:
//   Performs intra prediction for a 4x4 luma component.
//
//  Parameters:
//    pSrcDst     - pointer to the source and destination array
//    srcDstStep  - Step (in bytes) of the source and destination array,
//    predMode    - prediction mode, valid range [0, 8]
//    avilability - flag that specifies the availability of
//                  the samples for prediction.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     pSrcDst is NULL
//    ippStsStepErr        srcDstStep is less than 4
//    ippStsOutOfRangeErr  predMode is out of range [0,8]
*/

IPPAPI(IppStatus, ippiPredictIntra_4x4_H264_8u_C1IR, (
  Ipp8u*                    pSrcDst,
  Ipp32s                    srcdstStep,
  IppIntra4x4PredMode_H264  predMode,
  Ipp32s                    availability))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiPredictIntra_16x16_H264_8u_C1IR
//
//  Purpose:
//    Performs intra prediction for a 16x16 luma component.
//
//  Parameters:
//    pSrcDst     - pointer to the source and destination array
//    srcDstStep  - Step (in bytes) of the source and destination array,
//    predMode    - prediction mode, valid range [0,3]
//    avilability - flag that specifies the availability of
//                  the samples for prediction.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     pSrcDst is NULL
//    ippStsStepErr        srcDstStep is less than 16
//    ippStsOutOfRangeErr  predMode is out of range [0,3]
*/

IPPAPI(IppStatus, ippiPredictIntra_16x16_H264_8u_C1IR, (
  Ipp8u*                     pSrcDst,
  Ipp32s                     srcdstStep,
  IppIntra16x16PredMode_H264 predMode,
  Ipp32s                     availability))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiPredictIntraChroma8x8_H264_8u_C1IR
//
//  Purpose:
//    Perform intra prediction for an 8x8 chroma component.
//
//  Parameters:
//    pSrcDst     - pointer to the source and destination array
//    srcDstStep  - Step (in bytes) of the source and destination array,
//    predMode    - prediction mode, valid range [0,3]
//    avilability - flag that specifies the availability of
//                  the samples for prediction.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     pSrcDst is NULL
//    ippStsStepErr        srcDstStep is less than 8
//    ippStsOutOfRangeErr  predMode is out of range [0,3]
*/

IPPAPI(IppStatus, ippiPredictIntraChroma8x8_H264_8u_C1IR, (
  Ipp8u*                      pSrcDst,
  Ipp32s                      srcdstStep,
  IppIntraChromaPredMode_H264 predMode,
  Ipp32s                      availability))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiTransformDequantLumaDC_H264_16s_C1I
//    ippiTransformDequantChromaDC_H264_16s_C1I
//
//  Purpose:
//     Perform integer inverse transformation and dequantization
//     for 4x4 luma DC coefficients,
//     and 2x2 chroma DC coefficients respectively.
//
//  Parameters:
//     pSrcDst - pointer to initial coefficients and resultant DC,
//     QP      - quantization parameter.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     pSrcDst is NULL
//    ippStsOutOfRangeErr  QP is less than 1 or greater than 51
*/

IPPAPI(IppStatus, ippiTransformDequantLumaDC_H264_16s_C1I, (
  Ipp16s* pSrcDst,
  Ipp32s  QP))

IPPAPI(IppStatus, ippiTransformDequantChromaDC_H264_16s_C1I, (
  Ipp16s* pSrcDst,
  Ipp32s  QP))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDequantTransformResidual_H264_16s_C1I
//
//  Purpose:
//    Places a DC coefficient (if any) to its place,
//    Performs dequantization, integer inverse transformation and
//    shift by 6 bits for 4x4 block of residuals.
//
//  Parameters:
//    pSrcDst - pointer to the initial coefficients and resultant residuals,
//    step    - step (in bytes) of the source and destination array,
//    pDC     - pointer to the DC coefficient. If it is set to NULL, than
//              Inter 4x4 Inverse quantization is performed on the DC
//              coefficient in the top left corner of the macroblock,
//              otherwise function just gets it in the specified location;
//    AC      - flag that is not zero, if at least one AC coefficient exists,
//              and equals zero otherwise.
//    QP      - quantization parameter.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     pSrcDst is NULL
//    ippStsOutOfRangeErr  QP is less than 1 or greater than 51
//    ippStsStepErr        step is less than 8 respectively
*/


IPPAPI(IppStatus, ippiDequantTransformResidual_H264_16s_C1I, (
  Ipp16s* pSrcDst,
  Ipp32s  step,
  Ipp16s* pDC,
  Ipp32s  AC,
  Ipp32s  QP))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDequantTransformResidualAndAdd_H264_16s_C1I
//
//  Purpose:
//  Places a DC coefficient (if any) to its place,
//  Performs dequantization, integer inverse transformation and
//  shift by 6 bits for 4x4 block of residuals
//  with subsequent intra prediction or motion
//  compensation.
//
//
//  Parameters:
//    pPred       -  pointer to the reference 4x4 block, which is used for intra
//                                       prediction or motion compensation.
//    pSrcDst     -  pointer to the initial coefficients and resultant residuals (4x4
//                                       block) - array of size 16.
//    pDC         -  pointer to the DC coefficient. In the case of Intra 4x4
//                                       macroblock type pDC is set to NULL.
//    pDst        -  pointer to the destination 4x4 block.
//    PredStep    -  reference frame step in bytes.
//    DstStep     -  destination frame step in bytes.
//    QP          -  quantization parameter
//    AC                  -  flag that is not equal to zero, if at least one AC coefficient
//                                       exists, and is equal to zero otherwise.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     pSrcDst is NULL
//    ippStsOutOfRangeErr  QP is less than 1 or greater than 51
//
*/

IPPAPI(IppStatus, ippiDequantTransformResidualAndAdd_H264_16s_C1I,(
    const Ipp8u*  pPred,
          Ipp16s* pSrcDst,
          Ipp16s* pDC,
          Ipp8u*  pDst,
          Ipp32s  PredStep,
          Ipp32s  DstStep,
          Ipp32s  QP,
          Ipp32s  AC))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR
//
//  Purpose:
//     Performs deblocking filtering on the vertical edges of the
//     luma macroblock(16x16) in accordance with 8.7.2. of H.264 standard
//
//  Parameters:
//    pSrcDst    - pointer to the initial and resultant coefficients,
//    srcdstStep - step of the arrays,
//    pAlpha     - array of size 2 of Alpha Thresholds(values for external
//                 and internal vertical edge)
//    pBeta      - array of size 2 of Beta  Thresholds(values for external
//                 and internal vertical edge)
//    pTresholds - array of size 16 of Thresholds (TC0)(values for
//                 the left edge of each 4x4 block)
//    pBS        - array of size 16 of BS parameters
//
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation
//    and Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR, (
  Ipp8u*    pSrcDst,
  Ipp32s    srcdstStep,
  const Ipp8u*    pAlpha,
  const Ipp8u*    pBeta,
  const Ipp8u*    pThresholds,
  const Ipp8u*    pBs))

  /* ///////////////////////////////////////////////////////////////////////////
  //  Name:
  //    ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR
  //
  //  Purpose:
  //     Performs deblocking filtering on the vertical edges of the
  //     luma macroblock(16x16) in accordance with 8.7.2. of H.264 standard
  //
  //  Parameters:
  //    pSrcDst    - pointer to the initial and resultant coefficients,
  //    srcdstStep - step of the arrays,
  //    nAlpha     - Alpha Threshold (value for external vertical edge)
  //    nBeta      - Beta  Threshold (value for external vertical edge)
  //    pTresholds - array of size 16 of Thresholds
  //    pBS        - array of size 16 of BS parameters
  //
  //
  //  Notes:
  //    H.264 standard: JVT-G050. ITU-T Recommendation
  //    and Final Draft International Standard of Joint Video Specification
  //    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
  //
  //  Returns:
  //    ippStsNoErr          No error
  //    ippStsNullPtrErr     One of the pointers is NULL
  */

IPPAPI(IppStatus, ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR, (
  Ipp8u*    pSrcDst,
  Ipp32s    srcdstStep,
  const Ipp32u    nAlpha,
  const Ipp32u    nBeta,
  const Ipp8u*    pThresholds,
  const Ipp8u*    pBs))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR
//
//  Purpose:
//     Performs deblocking filtering on the horizontal edges of the
//     luma macroblock(16x16) in accordance with 8.7.2. of H.264 standard
//
//  Parameters:
//    pSrcDst    - pointer to the initial and resultant coefficients,
//    srcdstStep - step of the arrays,
//    pAlpha     - array of size 2 of Alpha Thresholds(values for external
//                 and internal vertical edge)
//    pBeta      - array of size 2 of Beta  Thresholds(values for external
//                 and internal vertical edge)
//    pTresholds - array of size 16 of Thresholds (TC0)(values for
//                 the left edge of each 4x4 block)
//    pBS        - array of size 16 of BS parameters
//
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//   (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR, (
  Ipp8u*    pSrcDst,
  Ipp32s    srcdstStep,
  const Ipp8u*    pAlpha,
  const Ipp8u*    pBeta,
  const Ipp8u*    pThresholds,
  const Ipp8u*    pBS))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR
//
//  Purpose:
//    Performs deblocking filtering on the vertical edges of the
//    chroma macroblock(8x8) in accordance with 8.7.2. of the H.264 standard
//
//  Parameters:
//    pSrcDst    - pointer to the initial and resultant coefficients,
//    srcdstStep - step of the arrays,
//    pAlpha     - array of size 2 of Alpha Thresholds(values for
//                 external and internal vertical edge)
//    pBeta      - array of size 2 of Beta  Thresholds(values for
//                 external and internal vertical edge)
//    pTresholds - array of size 16 of Thresholds (TC0)(values for
//                 the left edge of each 2x2 block)
//    pBS        - array of size 16 of BS parameters
//
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR, (
  Ipp8u*       pSrcDst,
  Ipp32s       srcdstStep,
  const Ipp8u*       pAlpha,
  const Ipp8u*       pBeta,
  const Ipp8u*       pThresholds,
  const Ipp8u*       pBS))

  /* ///////////////////////////////////////////////////////////////////////////
  //  Name:
  //    ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR
  //
  //  Purpose:
  //    Performs deblocking filtering on the vertical edges of the
  //    chroma macroblock(8x8) in accordance with 8.7.2. of the H.264 standard
  //
  //  Parameters:
  //    pSrcDst    - pointer to the initial and resultant coefficients,
  //    srcdstStep - step of the arrays,
  //    nAlpha     - Alpha Threshold (value for external vertical edge)
  //    nBeta      - Beta  Threshold (value for external vertical edge)
  //    pTresholds - array of size 16 of Thresholds
  //    pBS        - array of size 16 of BS parameters
  //
  //
  //  Notes:
  //    H.264 standard: JVT-G050. ITU-T Recommendation and
  //    Final Draft International Standard of Joint Video Specification
  //    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
  //
  //  Returns:
  //    ippStsNoErr          No error
  //    ippStsNullPtrErr     One of the pointers is NULL
  */

IPPAPI(IppStatus, ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR, (
  Ipp8u*       pSrcDst,
  Ipp32s       srcdstStep,
  const Ipp32u       nAlpha,
  const Ipp32u       nBeta,
  const Ipp8u*       pThresholds,
  const Ipp8u*       pBS))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR
//
//  Purpose:
//     Performs deblocking filtering on the horizontal edges of the
//     chroma macroblock(8x8) in accordance with 8.7.2. of H.264 standard
//
//  Parameters:
//    pSrcDst    - pointer to the initial and resultant coefficients,
//    srcdstStep - step of the arrays,
//    pAlpha     - array of size 2 of Alpha Thresholds (values for
//                 external and internal horizontal edge)
//    pBeta      - array of size 2 of Beta  Thresholds (values for
//                 external and internal horizontal edge)
//    pTresholds - array of size 16 of Thresholds (TC0) (values for
//                 the upper edge of each 2x2 block)
//    pBS        - array of size 16 of BS parameters (values for
//                 external and internal horizontal edge)
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
*/

IPPAPI(IppStatus, ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR, (
  Ipp8u*       pSrcDst,
  Ipp32s       srcdstStep,
  const Ipp8u*       pAlpha,
  const Ipp8u*       pBeta,
  const Ipp8u*       pThresholds,
  const Ipp8u*       pBS))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiInterpolateLuma_H264_8u_C1R
//
//  Purpose:
//    Performs interpolation for motion estimation of the luma component.
//
//  Parameters:
//    pSrc     - pointer to the source,
//    srcStep  - step of the source buffer in bytes
//    pDst     - pointer to destination (should be 16-bytes aligned),
//    dstStep  - step of the destination buffer in bytes,
//    dx, dy   - fractional parts of the motion vector in
//               1/4 pel units (0, 1, 2, or 3),
//    roiSize  - flag that specifies the dimensions of
//               the ROI(could be 16, 8 or 4 in each dimension).
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
//    ippStsStepErr        srcStep is less than 16
//    ippStsBadArg         dx or dy is out of range [0, 3]
//    ippStsSizeErr        roi.width or roi.height is not equal to 16, 8, or 4
*/

IPPAPI(IppStatus, ippiInterpolateLuma_H264_8u_C1R, (
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        IppiSize roiSize))


IPPAPI(IppStatus, ippiInterpolateLumaTop_H264_8u_C1R, (
        const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        Ipp32s   outPixels,
        IppiSize roiSize))

IPPAPI(IppStatus, ippiInterpolateLumaBottom_H264_8u_C1R, (
       const Ipp8u*   pSrc,
       Ipp32s   srcStep,
       Ipp8u*   pDst,
       Ipp32s   dstStep,
       Ipp32s   dx,
       Ipp32s   dy,
       Ipp32s   outPixels,
       IppiSize roiSize))




IPPAPI(IppStatus, ippiInterpolateBlock_H264_8u_P2P1R, (const Ipp8u *pSrc1,
                                                       const Ipp8u *pSrc2,
                                                       Ipp8u *pDst,
                                                       Ipp32u uWidth,
                                                       Ipp32u uHeight,
                                                       Ipp32s pitch))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    iippiInterpolateChroma_H264_8u_C1R
//
//  Purpose:
//    Performs interpolation for motion estimation of the chroma component.
//
//  Parameters:
//    pSrc     - pointer to the source,
//    srcStep  - step of the source buffer in bytes
//    pDst     - pointer to destination
//    dstStep  - step of the destination buffer in bytes,
//    dx, dy   - fractional parts of the motion vector in
//               1/8 pel units (0, 1, .., 7),
//    roiSiaze - flag that specifies the dimensions of
//               the ROI(could be 16, 8 or 4 in each dimension).
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
//    ippStsStepErr        srcStep is less than 8
//    ippStsBadArg         dx or dy is out of range [0,7]
//    ippStsSizeErr        roi.width or roi.height is not equal to 16, 8, or 4
*/

IPPAPI(IppStatus, ippiInterpolateChroma_H264_8u_C1R, (
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        IppiSize roiSize))

IPPAPI(IppStatus, ippiInterpolateChromaTop_H264_8u_C1R, (
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        Ipp32s   outPixels,
        IppiSize roiSize))

IPPAPI(IppStatus, ippiInterpolateChromaBottom_H264_8u_C1R, (
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        Ipp32s   outPixels,
        IppiSize roiSize))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiWeightedAverage_H264_8u_C1IR
//
//  Purpose:
//    Averages two blocks with weights (for weighted bi-directional
//    predictions) in accordance with 8.4.2.3.2 of the H.264 standard.
//    dst = Clip( ( w1*src1 + w2*src2 + (1<<(shift-1)) )>>shift + offset )
//
//  Parameters:
//    pSrc1            - pointer to 1st source (output of preceding functions),
//    pSrc2Dst         - pointer to 2nd source and result
//    srcDstStep       - step value in bytes,
//    weight1, weight2 - weights,
//    shift            - shift,
//    offset           - offset,
//    roiSize          - flag that specifies the dimensions of the ROI
//                       (could be 16, 8 or 4 in each dimension).
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
//    ippStsStepErr        srcDstStep is less than roi.width
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiWeightedAverage_H264_8u_C1IR, (
  const Ipp8u*   pSrc1,
        Ipp8u*   pSrc2Dst,
        Ipp32s   srcDstStep,
        Ipp32s   weight1,
        Ipp32s   weight2,
        Ipp32s   shift,
        Ipp32s   offset,
        IppiSize roiSize))

IPPAPI(IppStatus, ippiUniDirWeightBlock_H264_8u_C1R, (
        Ipp8u *pSrcDst,
        Ipp32u srcDstStep,
        Ipp32u ulog2wd,
        Ipp32s iWeight,
        Ipp32s iOffset,
        IppiSize roi
        ))

IPPAPI(IppStatus, ippiBiDirWeightBlock_H264_8u_P2P1R,(
        Ipp8u *pSrc1,
        Ipp8u *pSrc2,
        Ipp8u *pDst,
        Ipp32u srcStep,
        Ipp32u dstStep,
        Ipp32u ulog2wd,
        Ipp32s iWeight1,
        Ipp32s iOffset1,
        Ipp32s iWeight2,
        Ipp32s iOffset2,
        IppiSize roi
        ))

IPPAPI(IppStatus, ippiBiDirWeightBlockImplicit_H264_8u_P2P1R, (
        Ipp8u *pSrc1,
        Ipp8u *pSrc2,
        Ipp8u *pDst,
        Ipp32u srcStep,
        Ipp32u dstStep,
        Ipp32s iWeight1,
        Ipp32s iWeight2,
        IppiSize roi
        ))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiTransformPrediction_H264_8u16s_C1
//
//  Purpose:
//
//  Parameters:
//    pSrc             - pointer to source,
//    step             - source step in bytes,
//    pDst             - pointer to destination
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiTransformPrediction_H264_8u16s_C1, (
        Ipp8u   *pSrc,
        Ipp32s  step,
        Ipp16s  *pDst))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDequantTransformResidual_SISP_H264_16s_C1I
//
//  Purpose:
//
//  Parameters:
//    pSrcDst          - pointer to working data,
//    pPredictBlock    -
//    pDC              -
//    AC               -
//    qp               -
//    qs               -
//    Switch           -
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiDequantTransformResidual_SISP_H264_16s_C1I, (
        Ipp16s* pSrcDst,
        Ipp16s* pPredictBlock,
        Ipp16s* pDC,
        Ipp32s  AC,
        Ipp32s  qp,
        Ipp32s  qs,
        Ipp32s  Switch))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiTransformDequantChromaDC_SISP_H264_16s_C1I
//
//  Purpose:
//    The function performs integer inverse transformation and
//    dequantisation for  2x2 chroma DC coefficients.
//    It used for SI & SP frames.
//
//  Parameters:
//    pSrcDst          - pointer to working data,
//    pDCpredict       -
//    qp               -
//    qs               -
//    Switch           -
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     One of the pointers is NULL
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiTransformDequantChromaDC_SISP_H264_16s_C1I, (
        Ipp16s* pSrcDst,
        Ipp16s* pDCpredict,
        Ipp32s qp,
        Ipp32s qs,
        Ipp32s Switch))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaIntraMB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Intra Luma macroblock
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current MB which will be reconstructed
//      srcdstYStep         - plane step
//      pMBIntraTypes       - pointer to intra types for each subblock
//      cbp4x4              - coded block pattern
//      QP                  - quantizer
//      edgeType            - MB eadge type
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//      H.264 standard: JVT-G050. ITU-T Recommendation and
//      Final Draft International Standard of Joint Video Specification
//      (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiReconstructLumaIntraMB_H264_16s8u_C1R, (Ipp16s **ppSrcCoeff,
                                                              Ipp8u *pSrcDstYPlane,
                                                              Ipp32s srcdstYStep,
                                                              const IppIntra4x4PredMode_H264 *pMBIntraTypes,
                                                              const Ipp32u cbp4x4,
                                                              const Ipp32u QP,
                                                              const Ipp8u edgeType))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructChromaInterMB_H264_16s8u_P2R
//
//  Purpose:
//      Reconstruct Inter Chroma macroblock
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstUPlane       - pointer to current U plane which will be reconstructed
//      pSrcDstVPlane       - pointer to current V plane which will be reconstructed
//      srcdstUVStep        - plane step
//      cbp4x4              - coded block pattern
//      ChromaQP            - chroma quantizer
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr ChromaQP is less than 0 or greater than 39
//
//  Notes:
//      H.264 standard: JVT-G050. ITU-T Recommendation and
//      Final Draft International Standard of Joint Video Specification
//      (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiReconstructChromaInterMB_H264_16s8u_P2R, (Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstUPlane,
                                                                Ipp8u *pSrcDstVPlane,
                                                                const Ipp32u srcdstStep,
                                                                const Ipp32u cbp4x4,
                                                                const Ipp32u ChromaQP))

IPPAPI(IppStatus, ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R, (Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstYPlane,
                                                                Ipp32s srcdstYStep,
                                                                IppIntra4x4PredMode_H264 *pMBIntraTypes,
                                                                Ipp32u cbp4x2,
                                                                Ipp32u QP,
                                                                Ipp8u edgeType))

IPPAPI(IppStatus, ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R, (Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstUPlane,
                                                                Ipp8u *pSrcDstVPlane,
                                                                Ipp32u srcdstUVStep,
                                                                IppIntraChromaPredMode_H264 intra_chroma_mode,
                                                                Ipp32u cbp4x4,
                                                                Ipp32u ChromaQP,
                                                                Ipp8u edge_type_top,
                                                                Ipp8u edge_type_bottom))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructChromaIntraMB_H264_16s8u_P2R
//
//  Purpose:
//      Reconstruct Intra Chroma macroblock
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstUPlane       - pointer to current U plane which will be reconstructed
//      pSrcDstVPlane       - pointer to current V plane which will be reconstructed
//      srcdstUVStep        - plane step
//      intra_chroma_mode   - intra mode
//      cbp4x4              - coded block pattern
//      ChromaQP            - chroma quantizer
//      edge_type           - edge type
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr ChromaQP is less than 0 or greater than 39
//
//  Notes:
//      H.264 standard: JVT-G050. ITU-T Recommendation and
//      Final Draft International Standard of Joint Video Specification
//      (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiReconstructChromaIntraMB_H264_16s8u_P2R, (Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstUPlane,
                                                                Ipp8u *pSrcDstVPlane,
                                                                const Ipp32u srcdstUVStep,
                                                                const IppIntraChromaPredMode_H264 intra_chroma_mode,
                                                                const Ipp32u cbp4x4,
                                                                const Ipp32u ChromaQP,
                                                                const Ipp8u edge_type))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaInterMB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Inter Luma macroblock
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      srcdsYStep          - plane step
//      cbp4x4              - coded block pattern
//      QP                  - quantizer
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//      H.264 standard: JVT-G050. ITU-T Recommendation and
//      Final Draft International Standard of Joint Video Specification
//      (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiReconstructLumaInterMB_H264_16s8u_C1R, (Ipp16s **ppSrcCoeff,
                                                              Ipp8u *pSrcDstYPlane,
                                                              Ipp32u srcdstYStep,
                                                              Ipp32u cbp4x4,
                                                              Ipp32s QP))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Intra 16x16 Luma macroblock
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      srcdstYStep         - plane step
//      intra_luma_mode     - intra mode
//      cbp4x4              - coded block pattern
//      QP                  - quantizer
//      edge_type           - edge type
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//      H.264 standard: JVT-G050. ITU-T Recommendation and
//      Final Draft International Standard of Joint Video Specification
//      (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R, (Ipp16s **ppSrcCoeff,
                                                                   Ipp8u *pSrcDstYPlane,
                                                                   Ipp32u srcdstYStep,
                                                                   const IppIntra16x16PredMode_H264 intra_luma_mode,
                                                                   const Ipp32u cbp4x4,
                                                                   const Ipp32u QP,
                                                                   const Ipp8u edge_type))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Intra 16x16 Luma macroblock for high profile
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      srcdstYStep         - plane step
//      intra_luma_mode     - intra mode
//      cbp4x4              - coded block pattern
//      QP                  - quantizer
//      edge_type           - edge type
//      pQuantTable         - pointer to quantisation table
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//
*/

    IPPAPI(IppStatus, ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32u srcdstYStep,
            IppIntra16x16PredMode_H264 intra_luma_mode,
            Ipp32u cbp4x4,
            Ipp32u QP,
            Ipp8u edge_type,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Intra 4x4 Luma macroblock for high profile
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      srcdstYStep         - plane step
//      pMBIntraTypes       - pointer to intra types for each subblock
//      cbp4x4              - coded block pattern
//      QP                  - quantizer
//      edge_type           - edge type
//      pQuantTable         - pointer to quantisation table
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//
*/
    IPPAPI(IppStatus, ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra4x4PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp4x4,
            Ipp32u QP,
            Ipp8u edgeType,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaInter4x4MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Inter 4x4 Luma macroblock for high profile
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      srcdstYStep         - plane step
//      cbp4x4              - coded block pattern
//      QP                  - quantizer
//      pQuantTable         - pointer to quantisation table
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//
*/
    IPPAPI(IppStatus, ippiReconstructLumaInter4x4MB_H264_16s8u_C1R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32u srcdstYStep,
            Ipp32u cbp4x4,
            Ipp32s QP,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructChromaInter4x4MB_H264_16s8u_P2R
//
//  Purpose:
//      Reconstruct Inter Chroma 4x4 macroblock for high profile
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstUPlane       - pointer to current U plane which will be reconstructed
//      pSrcDstVPlane       - pointer to current V plane which will be reconstructed
//      srcdstUVStep        - plane step
//      cbp4x4              - coded block pattern
//      chromaQPU           - chroma quantizer for U plane
//      chromaQPV           - chroma quantizer for V plane
//      edge_type           - edge type
//      pQuantTableU        - pointer to quantisation table for U plane
//      pQuantTableV        - pointer to quantisation table for V plane
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr ChromaQP is less than 0 or greater than 39
//
//  Notes:
//
*/

    IPPAPI(IppStatus, ippiReconstructChromaInter4x4MB_H264_16s8u_P2R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstUPlane,
            Ipp8u *pSrcDstVPlane,
            Ipp32u srcdstUVStep,
            Ipp32u cbp4x4,
            Ipp32u chromaQPU,
            Ipp32u chromaQPV,
            Ipp16s *pQuantTableU,
            Ipp16s *pQuantTableV,
            Ipp8u bypass_flag))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R
//
//  Purpose:
//      Reconstruct Intra Chroma 4x4 macroblock for high profile
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstUPlane       - pointer to current U plane which will be reconstructed
//      pSrcDstVPlane       - pointer to current V plane which will be reconstructed
//      srcdstUVStep        - plane step
//      intra_chroma_mode   - intra mode
//      cbp4x4              - coded block pattern
//      edge_type           - edge type
//      chromaQPU           - chroma quantizer for U plane
//      chromaQPV           - chroma quantizer for V plane
//      pQuantTableU        - pointer to quantisation table for U plane
//      pQuantTableV        - pointer to quantisation table for V plane
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr ChromaQP is less than 0 or greater than 39
//
//  Notes:
*/
    IPPAPI(IppStatus, ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstUPlane,
            Ipp8u *pSrcDstVPlane,
            Ipp32u srcdstUVStep,
            IppIntraChromaPredMode_H264 intra_chroma_mode,
            Ipp32u cbp4x4,
            Ipp32u chromaQPU,
            Ipp32u chromaQPV,
            Ipp8u edge_type,
            Ipp16s *pQuantTableU,
            Ipp16s *pQuantTableV,
            Ipp8u bypass_flag))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Intra 8x8 Luma macroblock for high profile
//
//  Parameters:
//      ppSrcCoeff          - pointer to 8x8 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      srcdstYStep         - plane step
//      pMBIntraTypes       - pointer to intra types for each subblock
//      cbp8x8              - coded block pattern
//      QP                  - quantizer
//      edge_type           - edge type
//      pQuantTable         - pointer to quantisation table
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//
*/
    IPPAPI(IppStatus, ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra8x8PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp8x8,
            Ipp32u QP,
            Ipp8u edgeType,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaInter8x8MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Inter 8x8 Luma macroblock for high profile
//
//  Parameters:
//      ppSrcCoeff          - pointer to 8x8 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      srcdstYStep         - plane step
//      cbp8x8              - coded block pattern
//      QP                  - quantizer
//      pQuantTable         - pointer to quantisation table
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//
*/
    IPPAPI(IppStatus, ippiReconstructLumaInter8x8MB_H264_16s8u_C1R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32u srcdstYStep,
            Ipp32u cbp8x8,
            Ipp32s QP,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Intra 4x4 Luma macroblock for high profile in MBAFF mode
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      pMBIntraTypes       - pointer to intra types for each subblock
//      cbp4x2              - coded block pattern
//      QP                  - quantizer
//      edge_type           - edge type
//      pQuantTable         - pointer to quantisation table
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//
*/
    IPPAPI(IppStatus, ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra4x4PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp4x2,
            Ipp32u QP,
            Ipp8u edgeType,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R
//
//  Purpose:
//      Reconstruct Intra 8x8 Luma macroblock for high profile in MBAFF mode
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstYPlane       - pointer to current Y plane which will be reconstructed
//      pMBIntraTypes       - pointer to intra types for each subblock
//      cbp8x2              - coded block pattern
//      QP                  - quantizer
//      edge_type           - edge type
//      pQuantTable         - pointer to quantisation table
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr QP is less than 0 or greater than 51
//
//  Notes:
//
*/
    IPPAPI(IppStatus, ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra8x8PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp8x2,
            Ipp32u QP,
            Ipp8u edgeType,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R
//
//  Purpose:
//      Reconstruct Intra Chroma 4x4 macroblock for high profile in MBAFF mode
//
//  Parameters:
//      ppSrcCoeff          - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      pSrcDstUPlane       - pointer to current U plane which will be reconstructed
//      pSrcDstVPlane       - pointer to current V plane which will be reconstructed
//      srcdstUVStep        - plane step
//      intra_chroma_mode   - intra mode
//      cbp4x4              - coded block pattern
//      edge_type_top       - edge type top
//      edge_type_bottom    - edge type bottom
//      chromaQPU           - chroma quantizer for U plane
//      chromaQPV           - chroma quantizer for V plane
//      pQuantTableU        - pointer to quantisation table for U plane
//      pQuantTableV        - pointer to quantisation table for V plane
//      bypass_flag         - enable lossless coding when qpprime_y is zero
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrcDst is NULL
//      ippStsOutOfRangeErr ChromaQP is less than 0 or greater than 39
//
//  Notes:
*/
IPPAPI(IppStatus, ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R, (Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstUPlane,
            Ipp8u *pSrcDstVPlane,
            Ipp32u srcdstUVStep,
            IppIntraChromaPredMode_H264 intra_chroma_mode,
            Ipp32u cbp4x4,
            Ipp32u chromaQPU,
            Ipp32u chromaQPV,
            Ipp8u edge_type_top,
            Ipp8u edge_type_bottom,
            Ipp16s *pQuantTableU,
            Ipp16s *pQuantTableV,
            Ipp8u bypass_flag))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiInterpolateBlock_H264_8u_P3P1R
//
//  Purpose: interpolation block nHeight x nWidth
//
//
//  Parameters:
//      pSrc0             - pointer to first source
//      pSrc1             - pointer to second source
//      pDst              - pointer to destination
//      nWidth            - block width
//      nHeight           - block height
//      iPitchSrc0        - first source pitch
//      iPitchSrc1        - second source pitch
//      iPitchDst         - destination pitch
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrc0 or pSrc1 or pDst is NULL
//  Notes:
//
*/
IPPAPI(IppStatus, ippiInterpolateBlock_H264_8u_P3P1R, (const Ipp8u *pSrc1,
                                                       const Ipp8u *pSrc2,
                                                       Ipp8u *pDst,
                                                       Ipp32u uWidth,
                                                       Ipp32u uHeight,
                                                       Ipp32s iPitchSrc1,
                                                       Ipp32s iPitchSrc2,
                                                       Ipp32s iPitchDst))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiBiDirWeightBlock_H264_8u_P3P1R
//
//  Purpose: Apply specified weighting and offset to all samples of the blocks,
//  and combine them.
//      Sw = clip( (S0*weight0 + S1*weight1 + round) >> (ulog2wd+1) +
//                ((offset0 + offset1 + 1)>>1) )
//
//  Parameters:
//      pSrc1             - pointer to first source
//      pSrc2             - pointer to second source
//      pDst              - pointer to destination
//      nWidth            - block width
//      nHeight           - block height
//      nSrcPitch1        - first source pitch
//      nSrcPitch2        - second source pitch
//      nDstPitch         - destination pitch
//      ulog2wd           - log2 weight denominator
//      iWeight1          - weight coefficient for first source
//      iOffset1          - offset for first source
//      iWeight2          - weight coefficient for second source
//      iOffset2          - offset for second source
//      roi               - block size
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrc1 or pSrc2 or pDst is NULL
//  Notes:
//
*/
IPPAPI(IppStatus, ippiBiDirWeightBlock_H264_8u_P3P1R,( const Ipp8u *pSrc1,
       const Ipp8u *pSrc2,
       Ipp8u *pDst,
       Ipp32u nSrcPitch1,
       Ipp32u nSrcPitch2,
       Ipp32u nDstPitch,
       Ipp32u ulog2wd,
       Ipp32s iWeight1,
       Ipp32s iOffset1,
       Ipp32s iWeight2,
       Ipp32s iOffset2,
       IppiSize roi
    ))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiBiDirWeightBlockImplicit_H264_8u_P3P1R
//
//  Purpose: Implicit bidir prediction weighting using simplified weighting with
//    no offsets.
//        Sw = clip((S0*weight0 + S1*weight1 + 32) >> 6))
//
//  Parameters:
//      pDst              - pointer to destination
//      nWidth            - block width
//      nHeight           - block height
//      nSrcPitch1        - first source pitch
//      nSrcPitch2        - second source pitch
//      nDstPitch         - destination pitch
//      iWeight1          - weight coefficient for first source
//      iWeight2          - weight coefficient for second source
//      roi               - block size
//
//
//  Returns:
//      ippStsNoErr         No error
//      ippStsNullPtrErr    pSrc0 or pSrc1 or pDst is NULL
//  Notes:
//
*/
IPPAPI(IppStatus, ippiBiDirWeightBlockImplicit_H264_8u_P3P1R,(const Ipp8u *pSrc1,
       const Ipp8u *pSrc2,
       Ipp8u *pDst,
       Ipp32u nSrcPitch1,
       Ipp32u nSrcPitch2,
       Ipp32u nDstPitch,
       Ipp32s iWeight1,
       Ipp32s iWeight2,
       IppiSize roi
    ))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s
//
//  Purpose: Decode Chroma DC coefficients CAVLC coded
//
//  Parameters:
//          ppBitStream         - double pointer to current dword in bitstream(will be updated by function)
//              pOffset                 - pointer to offset in current dword(will be updated by function)
//              pNumCoeff               - output number of coefficients
//              ppPosCoefbuf    - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//              pTblCoeffToken  - chroma DC CoeffToken Table
//              ppTblTotalZerosCR - chroma DC TotalZeros Tables
//              ppTblRunBefore  - RunBefore Tables
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     if a pointer is NULL
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s, (Ipp32u **ppBitStream,
                                                             Ipp32s *pOffset,
                                                             Ipp16s *pNumCoeff,
                                                             Ipp16s **ppDstCoeffs,
                                                             const Ipp32s *pTblCoeffToken,
                                                             const Ipp32s **ppTblTotalZerosCR,
                                                             const Ipp32s **ppTblRunBefore))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiDecodeCAVLCCoeffs_H264_1u16s
//
//  Purpose:
//      Decode any non Chroma DC coefficients CAVLC coded
//
//  Parameters:
//      ppBitStream     - double pointer to current dword in bitstream(will be updated by function)
//      pOffset         - pointer to offset in current dword(will be updated by function)
//      pNumCoeff       - output number of coefficients
//      ppPosCoefbuf    - pointer to 4x4 block of coefficiens, if it's non zero(will be update by funcrion)
//      uVLCSelect      - predictor on number of CoeffToken Table
//      uMaxNumCoeff    - maximux coefficients in block(16 for Intra16x16, 15 for the rest)
//      pTblCoeffToken  - CoeffToken Tables
//      ppTblTotalZeros - TotalZeros Tables
//      ppTblRunBefore  - RunBefore Tables
//      pScanMatrix     - inverse scan matrix for coefficients in block
//
//  Returns:
//      ippStsNoErr       No error
//      ippStsNullPtrErr  if a pointer is NULL
//
//  Notes:
//      H.264 standard: JVT-G050. ITU-T Recommendation and
//      Final Draft International Standard of Joint Video Specification
//      (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiDecodeCAVLCCoeffs_H264_1u16s, (Ipp32u **ppBitStream,
                                                     Ipp32s *pOffset,
                                                     Ipp16s *pNumCoeff,
                                                     Ipp16s **ppDstCoeffs,
                                                     Ipp32u uVLCSelect,
                                                     Ipp16s uMaxNumCoeff,
                                                     const Ipp32s **ppTblCoeffToken,
                                                     const Ipp32s **ppTblTotalZeros,
                                                     const Ipp32s **ppTblRunBefore,
                                                     const Ipp32s *pScanMatrix))

IPPAPI(IppStatus, ippiDecodeExpGolombOne_H264_1u16s, (Ipp32u **ppBitStream,
                                                      Ipp32s *pBitOffset,
                                                      Ipp16s *pDst,
                                                      Ipp8u isSigned))

IPPAPI( IppStatus, ippiExpandPlane_H264_8u_C1R,   ( Ipp8u *StartPtr,
                                                    Ipp32u uFrameWidth,
                                                    Ipp32u uFrameHeight,
                                                    Ipp32u uPitch,
                                                    Ipp32u uPels,
                                                    IppvcFrameFieldFlag uFrameFieldFlag))

/* ///////////////////////////////////////////////////////////////////////////
//                     H.264 Video Encoder Functions
//////////////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeCoeffsCAVLC_H264_16s
//
//  Purpose: Calculates characteristics of 4X4 block for CAVLC encoding.
//
//  Parameters:
//              pSrc                            Pointer to 4x4 block - array of size 16.
//              AC                                      Flag, equal to zero in the cases of Luma Intra 16x16 AC block
//                                                      or Chroma AC block, and is not equal to zero otherwise.
//              pScanMatrix                     Scan matrix for coefficients in block (array of size 16).
//              Count                           Position of the last non-zero block coefficient in the scanning sequence.
//              Traling_One                     The number of trailing ones transform coefficient levels
//                                                      in a range[0;3]. This argument is calculated by the function.
//              Traling_One_Signs       Code that describes signs of trailing ones.
//                                                      (Trailing_One 1 -      i)-bit in this code corresponds to a sign
//                                                      of i-trailing one in the current block. In this code 1 indicates
//                                                      negative value, 0 positive value. This  argument is calculated
//                                                      by the function.
//              NumOutCoeffs            The number of non-zero coefficients in block (including trailing
//                                                      ones). This argument is calculated by the function.
//              TotalZeros                      The number of zero coefficients in block (except trailing zeros). This
//                                                      argument is calculated by the function.
//              pLevels                         Pointer to an array of size 16 that contains non-zero quantized
//                                                      coefficients of the current block (except trailing ones) in reverse scan
//                                                      matrix order.
//              pRuns                           Pointer to an array of size 16 that contains runs before non-zero
//                                                      quantized coefficients (including trailing ones) of the current block in
//                                                      reverse scan matrix order (except run before the first non-zero
//                                                      coefficient in block, which can be calculated using TotalZeros).
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     if a pointer is NULL
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/
IPPAPI(IppStatus, ippiEncodeCoeffsCAVLC_H264_16s, (
        Ipp16s  *pSrc,
        Ipp8u   AC,
        Ipp32s  *pScanMatrix,
        Ipp8u   Count,
        Ipp8u   *Trailing_Ones,
        Ipp8u   *Trailing_One_Signs,
        Ipp8u   *NumOutCoeffs,
        Ipp8u   *TotalZeros,
        Ipp16s  *Levels,
        Ipp8u   *Runs
))




/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeChromaDcCoeffsCAVLC_H264_16s
//
//  Purpose: Calculates characteristics of 2X2 Chroma DC for CAVLC encoding.
//
//  Parameters:
//              pSrc                            Pointer to 2x2 block - array of size 4.
//              Traling_One                     The number of trailing ones transform coefficient levels
//                                                      in a range[0;3]. This argument is calculated by the function.
//              Traling_One_Signs       Code that describes signs of trailing ones.
//                                                      (Trailing_One 1 -      i)-bit in this code corresponds to a sign
//                                                      of i-trailing one in the current block. In this code 1 indicates
//                                                      negative value, 0  positive value. This  argument is calculated
//                                                      by the function.
//              NumOutCoeffs            The number of non-zero coefficients in block (including trailing
//                                                      ones). This argument is calculated by the function.
//              TotalZeros                      The number of zero coefficients in block (except trailing zeros). This
//                                                      argument is calculated by the function.
//              pLevels                         Pointer to an array of size 4 that contains non-zero quantized
//                                                      coefficients of the current block (except trailing ones) in reverse scan
//                                                      matrix order.
//              pRuns                           Pointer to an array of size 4 that contains runs before non-zero
//                                                      quantized coefficients (including trailing ones) of the current block in
//                                                      reverse scan matrix order (except run before the first non-zero
//                                                      coefficient in block, which can be calculated using TotalZeros).
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     if a pointer is NULL
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/

IPPAPI(IppStatus, ippiEncodeChromaDcCoeffsCAVLC_H264_16s, (
        Ipp16s  *pSrc,
        Ipp8u   *pTrailingOnes,
        Ipp8u   *pTrailingOneSigns,
        Ipp8u   *pNumOutCoeffs,
        Ipp8u   *pTotalZeros,
        Ipp16s  *pLevels,
        Ipp8u   *pRuns
))
/*
//  Name:
//    ippiTransformQuantChromaDC_H264_16s_C1I
//
//  Purpose: This function performs forward transform (if it's necessary) and quantization
//  for 2x2 DC Croma block
//
//  Parameters:
//      pSrcDst                 Pointer to 2x2 chroma DC block - source & destination array of size 4
//      pTBlock                 Pointer to 2x2 transformed chroma DC block - source or destination array of size 4
//      QPCroma                 Quantization parameter for chroma. It's in range [0,39]
//      NumLevels               Pointer to value, which contains:  a negative value of a number of non-zero
//                              elements in block after quantization (in the case of the first quantized element
//                              in block is not equal zero)
//                              a number of non-zero elements in block after quantization (in the case of the
//                              first quantized element in block is equal zero)
//                              This value is calculated by function.
//      Intra                   Flag that is equal 1 in the case of Intra slice, 0 otherwise.
//      NeedTransform           Flag that is equal 1 if transforming process is used. This flag is equal 0
//                              if transforming process is not used.
//
//  Returns:
//              ippStsNoErr                     No error
//              ippStsNullPtrErr                pointers are NULL
//              ippStsOutOfRangeErr             QPCroma >39
//              ippStsScaleRangeErr                     if any coefficient after quantization > MAX_CAVLC_LEVEL_VALUE


*/

IPPAPI(IppStatus, ippiTransformQuantChromaDC_H264_16s_C1I, (
        Ipp16s* pSrcDst,
        Ipp16s* pTBlock,
        Ipp32s  QPCroma,
        Ipp8s*  pNumLevels,
        Ipp8u   intra,
        Ipp8u   needTransform
))

/*
//  Name:
//    ippiTransformQuantLumaDC_H264_16s_C1I
//
//  Purpose:
//    This function performs forward transform (if it's necessary) and quantization
//    for 4x4 DC Luma block.
//
//  Parameters:
//    pSrcDst       Pointer to 4x4 luma DC block - source & destination array of size 4
//    pTBlock       Pointer to 4x4 transformed luma DC block - source or destination array of size 4
//    QP            Quantization parameter for luma. It's in range [0,51]
//    NumLevels     Pointer to value, which contains:
//                  a negative value of a number of non-zero elements in block after
//                  quantization (in the case of the first quantized element in block is not equal zero)
//                  a number of non-zero elements in block after quantization (in the case
//                  of the first quantized element in block is equal zero)
//                  This value is calculated by function.
//    NeedTransform Flag that is equal 1 if transforming process is used. This flag is equal 0 if transforming process is not used.
//    pScanMatrix   Scan matrix for coefficients in block (array of size 16)
//    LastCoeff     Position of the last non-zero coefficient in block after quantization. This value is calculated by function.
//
//
//  Returns:
//              ippStsNoErr                     No error
//              ippStsNullPtrErr                pointers are NULL
//              ippStsOutOfRangeErr             QP >51 or QP<0
//              ippStsScaleRangeErr                     if any coefficient after quantization > MAX_CAVLC_LEVEL_VALUE


*/

IPPAPI(IppStatus, ippiTransformQuantLumaDC_H264_16s_C1I, (
        Ipp16s* pSrcDst,
        Ipp16s* pTBlock,
        Ipp32s  QP,
        Ipp8s*  pNumLevels,
        Ipp8u   needTransform,
        Ipp16s* pScanMatrix,
        Ipp8u*  pLastCoeff

))
/*
//  Name:
//    ippiTransformQuantResidual_H264_16s_C1I
//
//  Purpose:
//    This function performs forward transform and quantization for 4x4 residual block.
//
//  Parameters:
//    pSrcDst       Pointer to 4x4 residual block - source & destination array of size 16
//    QP            Quantization parameter for luma or for chroma. It's in range [0,51] or [0,39]
//    NumLevels     Pointer to value, which contains:
//                  a negative value of a number of non-zero elements in block after quantization
//                  (in the case of the first quantized element in block is not equal zero)
//                  a number of non-zero elements in block after quantization (in the case
//                  of the first quantized element in block is equal zero)
//                  This value is calculated by function.
//    Intra                   Flag that is equal 1 in the case of Intra slice, 0 otherwise.
//    pScanMatrix         Scan matrix for coefficients in block (array of size 16)
//    LastCoeff           Position of the last non-zero coefficient in block after quantization. This value is calculated by function.
//
//
//  Returns:
//              ippStsNoErr                     No error
//              ippStsNullPtrErr                pointers are NULL
//              ippStsOutOfRangeErr             QP >51 or QP<0
//              ippStsScaleRangeErr                     if any coefficient after quantization > MAX_CAVLC_LEVEL_VALUE


*/
IPPAPI(IppStatus, ippiTransformQuantResidual_H264_16s_C1I , (
        Ipp16s* pSrcDst,
        Ipp32s  QP,
        Ipp8s*  pNumLevels,
        Ipp8u   intra,
        Ipp16s* pScanMatrix,
        Ipp8u*  pLastCoeff))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiYCrCb411ToYCbCr422_5MBDV_16s8u_P3C2R
//    ippiYCrCb411ToYCbCr422_ZoomOut2_5MBDV_16s8u_P3C2R
//    ippiYCrCb411ToYCbCr422_ZoomOut4_5MBDV_16s8u_P3C2R
//    ippiYCrCb411ToYCbCr422_ZoomOut8_5MBDV_16s8u_P3C2R
//
//  Purpose:
//    Convert a YCrCb411 macro blocks to the YCbCr422 macro blocks.
//    Reduce size of dst image in 1/2/4/8 times accordingly.
//
//  Parameters:
//    pSrc        array of pointers to the five source macro blocks
//    pDst        array of pointers to the five destination macro blocks
//    dstStep     step for the destination image
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pSrc pDst is NULL
*/
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_ZoomOut2_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_ZoomOut4_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_ZoomOut8_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5] ))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiYCrCb411ToYCbCr422_EdgeDV_16s8u_P3C2R
//    ippiYCrCb411ToYCbCr422_ZoomOut2_EdgeDV_16s8u_P3C2R
//    ippiYCrCb411ToYCbCr422_ZoomOut4_EdgeDV_16s8u_P3C2R
//    ippiYCrCb411ToYCbCr422_ZoomOut8_EdgeDV_16s8u_P3C2R
//
//  Purpose:
//    Converts a YCrCb411 macro block to the YCbCr422 macro block at the right edge of destination image.
//    Reduce size of dst image in 1/2/4/8 times accordingly.
//
//  Parameters:
//    pSrc        pointer to the source macro block
//    pDst        pointer to the destination macro block
//    dstStep     step for the destination image
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pSrc pDst is NULL
*/
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_EdgeDV_16s8u_P3C2R,(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep ))
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_ZoomOut2_EdgeDV_16s8u_P3C2R,(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep ))
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_ZoomOut4_EdgeDV_16s8u_P3C2R,(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep ))
IPPAPI(IppStatus, ippiYCrCb411ToYCbCr422_ZoomOut8_EdgeDV_16s8u_P3C2R,(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep ))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiYCrCb420ToYCbCr422_5MBDV_16s8u_P3C2R
//    ippiYCrCb420ToYCbCr422_ZoomOut2_5MBDV_16s8u_P3C2R
//    ippiYCrCb420ToYCbCr422_ZoomOut4_5MBDV_16s8u_P3C2R
//    ippiYCrCb420ToYCbCr422_ZoomOut8_5MBDV_16s8u_P3C2R
//
//  Purpose:
//    Convert a YCrCb420 macro blocks to the YCbCr422 macro blocks.
//    Reduce size of dst image in 1/2/4/8 times accordingly.
//
//  Parameters:
//    pSrc        array of pointers to the five source macro blocks
//    pDst        array of pointers to the five destination macro blocks
//    dstStep     step for the destination image
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pSrc pDst is NULL
*/
IPPAPI(IppStatus, ippiYCrCb420ToYCbCr422_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb420ToYCbCr422_ZoomOut2_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb420ToYCbCr422_ZoomOut4_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb420ToYCbCr422_ZoomOut8_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiYCrCb422ToYCbCr422_5MBDV_16s8u_P3C2R
//    ippiYCrCb422ToYCbCr422_ZoomOut2_5MBDV_16s8u_P3C2R
//    ippiYCrCb422ToYCbCr422_ZoomOut4_5MBDV_16s8u_P3C2R
//    ippiYCrCb422ToYCbCr422_ZoomOut8_5MBDV_16s8u_P3C2R
//
//  Purpose:
//    Convert a YCrCb422 macro blocks to the YCbCr422 macro blocks.
//    Reduce size of dst image in 1/2/4/8 times accordingly.
//
//  Parameters:
//    pSrc        array of pointers to the five source macro blocks
//    pDst        array of pointers to the five destination macro blocks
//    dstStep     step for the destination image
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pSrc pDst is NULL
*/
IPPAPI(IppStatus, ippiYCrCb422ToYCbCr422_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb422ToYCbCr422_ZoomOut2_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb422ToYCbCr422_ZoomOut4_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5], int dstStep ))
IPPAPI(IppStatus, ippiYCrCb422ToYCbCr422_ZoomOut8_5MBDV_16s8u_P3C2R,(const Ipp16s* pSrc[5], Ipp8u* pDst[5] ))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      ippiDeinterlaceFilterTriangle_8u_C1R
//
//  Purpose:
//      This function deinterlaces video plane.
//      The function performs triangle filtering of the image to remove interlacing
//      flicker effect that arises when analogue interlaced TV data is
//      viewed on a computer monitor.
//
//  Parameters:
//      pSrc            Pointer to the source video plane.
//      srcStep         Step through the source video plane.
//      pDst            Pointer to the destination video plane.
//      dstStep         Step through the destination video plane.
//      roiSize         Size of ROI. Height should be greater than 3.
//      centerWeight    Weight of filtered pixel, must lie within the range from 0 to 256.
//      layout          Plane layout, required when the plane is only a part of the frame.
//                      Takes the following values:
//                      IPP_UPPER for the first slice
//                      IPP_CENTER for the middle slices
//                      IPP_LOWER for the last slice
//                      IPP_LOWER && IPP_UPPER && IPP_CENTER for the image that is not  sliced.
//
//  Returns:
//      ippStsNoErr     Indicates no error.
//      ippStsNullPtrErr Indicates an error when at least one input pointer is NULL.
//      ippStsSizeErr   Indicates an error when roiSize  has a field with zero or negative value.
//      ippStsBadArgErr Indicates invalid argument.
*/

IPPAPI(IppStatus, ippiDeinterlaceFilterTriangle_8u_C1R, (
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        IppiSize roiSize,
        Ipp32u   centerWeight,
        Ipp32u   layout))

/* /////////////////////////////////////////////////////////////////////////////
//                   General Color Conversion
///////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Name:       ippiCbYCr422ToYCbCr420Rotate_8u_C2P3R
//  Purpose:    Converts a 2-channel YUY2 image to the I420(IYUV) image
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if srcRoi.width < 2 || srcRoi.height < 2
//    ippStsDoubleSize         If srcRoi.width and srcRoi.height are not multiples of 2,
//                             the function reduces the values to the nearest multiples of 2.
//  Arguments:
//    pSrc                    pointer to the source image
//    srcStep                 step for the source image
//    pDst                    array of pointers to the components of the destination image
//    dstStep                 array of steps values for every component
//    srcRoi                  region of interest of src image to be processed, in pixels,
//                            and roi of dst image you must calculate.
//    rotation                rotation control parameter; must be of
//                            the following pre-defined values:
//                            IPPVC_ROTATE_90CCW, IPPVC_ROTATE_90CW,
//                            IPPVC_ROTATE_180 or IPPVC_ROTATE_DISABLE,
*/
IPPAPI(IppStatus,ippiCbYCr422ToYCbCr420_Rotate_8u_C2P3R,(const Ipp8u* pSrc, int srcStep, IppiSize srcRoi,Ipp8u *pDst[3], int dstStep[3],
           int rotation ))
IPPAPI(IppStatus,ippiCbYCr422ToYCbCr420_Rotate_8u_P3R,(const Ipp8u* pSrc[3], int srcStep[3], IppiSize srcRoi,Ipp8u *pDst[3], int dstStep[3],
           int rotation ))
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Name:            ippiResizeCCRotate_8u_C2R
// Description:      synthesizes a low-resolution preview image for
//                   high-resolution video  or still capture applications.
//                   combines scale reduction(2:1,4:1 or 8:1), color space
//                   conversion and rotation into a a single function
// Input Arguments:
//                   pSrc            pointer to the source image. Input byte ordering is Cb Y Cr Y.
//                   srcStep         step for the source image
//                   pDst            pointer to the destination image
//                   dstStep         step for the destination image
//                   srcRoi          region of interest of src image to be processed, in pixels,
//                                   and roi of dst image you must calculate.
//                   zoomFactor      parameter, indicating downscale factor, takes values 2, 4 or 8 for 2:1,4:1,and 8:1 downscale respectively.
//                   interpolation   type of interpolation to
//                                     perform resampling of the input image
//                                     The following are currently supported:
//                                     IPPI_INTER_NN      nearest neighbor interpolation
//                                     IPPI_INTER_LINEAR  linear interpolation
//                   colorConversion color conversion control parameter,
//                                     must be set to one of the following
//                                     pre-defined values:
//                                                 IPPVC_CbYCr422ToBGR565
//                                     IPPVC_CbYCr422ToBGR565
//                   rotation         rotation control parameter,must be of
//                                     the following pre-defined values:
//                                     IPPVC_ROTATE_90CCW, IPPVC_ROTATE_90CW,
//                                     IPPVC_ROTATE_180 or IPPVC_ROTATE_DISABLE,
// Output Arguments: pDst             pointer to the start of the buffer
//                                     containing the, resized, color-converted,
//                                     and rotated output image.
//
// Returns:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         one or more pointers are NULL
//    ippStsSizeErr            srcRoi.width  < zoomFactor
//      ippStsSizeErr            srcRoi.height < zoomFactor
//    ippStsInterpolationErr   invalid values of the interpolation control parameter.
//    ippStsResizeFactorErr    invalid values of the zoomFactor control parameter.
//    ippStsBadArgErr          invalid values of the rotation control parameter.
//    ippStsDoubleSize         if srcRoi.width and srcRoi.height are not multiples of 2,
//                             the function reduces the values to the nearest multiples of 2.
*/
IPPAPI(IppStatus, ippiResizeCCRotate_8u_C2R,(const Ipp8u* pSrc, int srcStep, IppiSize srcRoi,
       Ipp16u *pDst,int dstStep, int zoomFactor,int interpolation,int colorConversion,int rotation ))
/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiGenScaleLevel8x8_H264_8u16s_D2
//
//  Purpose:
//  Performs quantization including 8x8 transform normalization
//
//  Parameters:
//  pSrcInvScaleMatrix   - Pointer to an original inverse scaling matrix for 8x8 transform.
//  SrcStep              - Step of the pSrcInvScaleMatrix in bytes.
//  pDstInvScaleMatrix   - Pointer to a destination inverse scaling matrix -- array of size 64.
//  pDstScaleMatrix      - Pointer to a destination forward scaling matrix - array of size 64.
//  Qp_rem               - Reminder from an integer division of quantization parameter by 6.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     at least one of the pointers is NULL
//    ippStsQPErr          Qp_rem is less than 0 or greater than 5
//
*/

IPPAPI(IppStatus, ippiGenScaleLevel8x8_H264_8u16s_D2, (
    const Ipp8u  *pSrcInvScaleMatrix,
    int           SrcStep,
    Ipp16s       *pDstInvScaleMatrix,
    Ipp16s       *pDstScaleMatrix,
    int           Qp_rem))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiTransformLuma8x8Fwd_H264_16s_C1I
//
//  Purpose:
//  Performs forward 8x8 transform for a 8x8 Luma block without normalization.
//
//  Parameters:
//    pSrcDst     -  pointer to the initial 8x8 Luma block and resultant coefficients
//                   (array of size 64).
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     at least one of the pointers is NULL
*/
IPPAPI(IppStatus, ippiTransformLuma8x8Fwd_H264_16s_C1I, (Ipp16s  *pSrcDst))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantLuma8x8_H264_16s_C1
//
//  Purpose:
//  Performs quantization including 8x8 transform normalization
//
//  Parameters:
//    pSrc             -  Pointer to Luma block coefficients - array of size 64.
//    pDst             -  Pointer to quantized and normalized coefficients -
//                        array of size 64.
//    Qp6              -  Quantization parameter divided by 6
//    Intra            -  Flag, 1 if the slice is intra and 0 otherwise.
//    pScanMatrix      -  Pointer to a scan matrix for the coefficients in the block (array
//                        of size 64)
//    pScaleLevels     -  Pointer to a scale level matrix taking into account 8x8 transform
//                        normalization.
//    pNumLevels       -  Pointer to a value which contains:
//                         - a negative value of a number of non-zero elements in block after
//                           quantization (when the first quantized element in block is not equal
//                           to zero),
//                         - a number of non-zero elements in block after quantization (when
//                           the first quantized element in block is equal to zero).
//    pLastCoeff       -  Position of the last (in order of pScanMatrix) non-zero coefficient
//                        in block after quantization. This value is calculated by the function.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     at least one of the pointers is NULL
//    ippStsQPErr          Qp6 is less than 0 or greater than 8
//
//  NOTE:
//    pSrc and pDst will usually point to the same memory region!
*/
IPPAPI(IppStatus, ippiQuantLuma8x8_H264_16s_C1, (
    const Ipp16s *pSrc,
    Ipp16s       *pDst,
    int           Qp6,
    int           Intra,
    const Ipp16s *pScanMatrix,
    const Ipp16s *pScaleLevels,
    int          *pNumLevels,
    int          *pLastCoeff))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiQuantLuma8x8Inv_H264_16s_C1I
//
//  Purpose:
//  Performs dequantization including inverse 8x8 transform normalization
//
//  Parameters:
//    pSrcDst          -  Pointer to a Luma block coefficients - source and destination
//                        array of size 64.
//    Qp6              -  Quantization parameter divided by 6
//    pInvLevelScale   -  Pointer to an inverse scale levels matrix.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     at least one of the pointers is NULL
//    ippStsQPErr          Qp6 is less than 0 or greater than 8 or
//
*/
IPPAPI(IppStatus, ippiQuantLuma8x8Inv_H264_16s_C1I, (
    Ipp16s       *pSrcDst,
    int           Qp6,
    const Ipp16s *pInvLevelScale))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiTransformLuma8x8InvAddPred_H264_16s8u_C1R
//
//  Purpose:
//  Performs inverse 8x8 transform for a 8x8 Luma block with subsequent intra
//  prediction or motion compensation, coefficients are assumed to be pre-normalized.
//
//  Parameters:
//    pPred       -  pointer to the reference 8x8 block, which is used for intra
//                   prediction or motion compensation.
//    PredStep    -  reference frame step in bytes.
//    pSrcDst     -  pointer to the initial coefficients and buffer for the computations
//                   (8x8 block) - array of size 64.
//    pDst        -  pointer to the destination 8x8 block.
//    DstStep     -  destination frame step in bytes.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     at least one of the pointers is NULL
//
*/
IPPAPI(IppStatus, ippiTransformLuma8x8InvAddPred_H264_16s8u_C1R, (
    const Ipp8u *pPred,
    int          PredStep,
    Ipp16s      *pSrcDst,
    Ipp8u       *pDst,
    int          DstStep))


#if defined __cplusplus
}
#endif

#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif

#endif /* __IPPVC_H__ */

/* End of file ippvc.h*/
