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

#include "umc_h264_dec_defs_yuv.h"
#include "ippvc.h"

static
IppStatus ippiExpandPlane_H264(Ipp8u *StartPtr,
                               Ipp32u uFrameWidth,
                               Ipp32u uFrameHeight,
                               Ipp32u uPitch,
                               Ipp32u uPels)
{
    Ipp32u i;
    Ipp8u *pSrcDst;

    /* check error(s) */
    if ((uFrameHeight & 1) || (2 > uFrameHeight))
        return ippStsSizeErr;

    if (0 == uFrameWidth)
        return ippStsSizeErr;
    if (0 == uPels)
        return ippStsNoErr;

    /* Area (2) - sides */
    pSrcDst = StartPtr;

    for (i = 0; i < uFrameHeight; i += 1)
    {
        /* set left site */
        ippsSet_8u_x(pSrcDst[0], pSrcDst - uPels, uPels);
        /* set right site */
        ippsSet_8u_x(pSrcDst[uFrameWidth - 1], pSrcDst + uFrameWidth, uPels);
        pSrcDst  += uPitch;
    }

    return ippStsNoErr;

} /* IPPFUN(IppStatus, ippiExpandPlane_H264_8u_C1R, (Ipp8u *StartPtr, */

namespace UMC
{

H264DecYUVBufferPadded::H264DecYUVBufferPadded()
    : m_pAllocatedBuffer(0)
    , m_allocatedSize(0)
    , m_pBuffer(0)
    , m_lumaSize(0, 0)
    , m_pitch(0)

{
}

H264DecYUVBufferPadded::~H264DecYUVBufferPadded()
{
    deallocate();
}

void H264DecYUVBufferPadded::deallocate()
{
    m_allocatedSize = 0;
    m_pBuffer = 0;
    clear();

    m_lumaSize = sDimensions(0, 0);
    m_pitch = 0;

    if (m_pAllocatedBuffer)
    {
        ippsFree_x(m_pAllocatedBuffer);
        m_pAllocatedBuffer = 0;
    }
}

void H264DecYUVBufferPadded::conditionalDeallocate(const sDimensions &dim,Ipp8u bpp,Ipp8u chroma_format)
{
    if (dim != lumaSize() || m_bpp!=bpp || m_chroma_format!=chroma_format)
        deallocate();
}

enum
{
    DATA_ALIGN                  = 64,
    LUMA_PADDING                = YUV_Y_PADDING,
    CHROMA_PADDING              = YUV_UV_PADDING
};

Status H264DecYUVBufferPadded::allocate(const sDimensions &lumaSize,Ipp8u bpp,Ipp8u chroma_format)
{
    Ipp32u newSize;
    Ipp32u nPitch;
    m_bpp = bpp;
    m_chroma_format = chroma_format;
    if (bpp<8 || bpp>16)  return UMC_UNSUPPORTED;
    if (chroma_format>3)  return UMC_BAD_FORMAT;
    static int chroma_height_mult[]={1,1,2,4};
    static int chroma_width_mult[]={1,1,1,2};
    // Y plane dimensions better be even, so width/2 correctly gives U,V size
    // YUV_ALIGNMENT must be a power of 2.  Since this is unlikely to change,
    // and since checking for a power of 2 is painful, let's just put a simple
    // VM_ASSERT here, that can be updated as needed for other powers of 2.
    nPitch = align_value<Ipp32u> (lumaSize.width + LUMA_PADDING * 2, DATA_ALIGN);
    newSize = nPitch * lumaSize.height +
        nPitch * (lumaSize.height*chroma_height_mult[chroma_format] / 2);

    if (m_pAllocatedBuffer)
        ippsFree_x(m_pAllocatedBuffer);

        m_pAllocatedBuffer = (Ipp8u *) ippsMalloc_8u_x(MAX(1, newSize + DATA_ALIGN * 2)*((bpp>8)+1));
        if (!m_pAllocatedBuffer)
        {
            // Reset all our state variables
            deallocate();
            return UMC_ALLOC;
        }
        m_allocatedSize = newSize;
        m_lumaSize = lumaSize;
        m_pitch = nPitch;
        //update advanced pointers

        m_pYPlaneAdv = align_pointer<Ipp16u *> (m_pAllocatedBuffer + LUMA_PADDING, DATA_ALIGN);
        m_pUPlaneAdv = align_pointer<Ipp16u *> (m_pAllocatedBuffer + CHROMA_PADDING * chroma_width_mult[chroma_format] +
        nPitch * lumaSize.height, DATA_ALIGN);
        if (chroma_format==3)
            m_pVPlaneAdv = align_pointer<Ipp16u *> (m_pUPlaneAdv + nPitch * lumaSize.height, DATA_ALIGN);
        else
            m_pVPlaneAdv = m_pUPlaneAdv + nPitch / 2;

        //update typical pointers as well
        m_pYPlane = align_pointer<Ipp8u *> (m_pAllocatedBuffer + LUMA_PADDING, DATA_ALIGN);
        m_pUPlane = align_pointer<Ipp8u *> (m_pAllocatedBuffer + CHROMA_PADDING * chroma_width_mult[chroma_format] +
        nPitch * lumaSize.height, DATA_ALIGN);
        if (chroma_format==3)
            m_pVPlane = align_pointer<Ipp8u *> (m_pUPlane + nPitch * lumaSize.height, DATA_ALIGN);
        else
            m_pVPlane = m_pUPlane + nPitch / 2 ;

    return UMC_OK;
}


Status
H264DecYUVWorkSpace::allocate(const sDimensions &lumaSize,Ipp8u bpp,Ipp8u chroma_format)
{
    sDimensions paddedSize;
        // rounded up to an integral number of macroblocks

    paddedSize.width  = (lumaSize.width  + 15) & ~15;
    paddedSize.height = (lumaSize.height + 15) & ~15;

    clearState();

    Status ps = H264DecYUVBufferPadded::allocate(paddedSize,bpp,chroma_format);

    if (ps == UMC_OK)
    {
        m_macroBlockSize.width  = paddedSize.width  >> 4;
        m_macroBlockSize.height = paddedSize.height >> 4;

        //m_subBlockSize.width    = m_macroBlockSize.width  << 2;
        //m_subBlockSize.height   = m_macroBlockSize.height << 2;
    }

    return ps;
}

void
H264DecYUVWorkSpace::conditionalDeallocate(const sDimensions &dim,Ipp8u bpp,Ipp8u chroma_format)
{
    sDimensions paddedSize;
        // rounded up to an integral number of macroblocks

    paddedSize.width  = (dim.width  + 15) & ~15;
    paddedSize.height = (dim.height + 15) & ~15;

    H264DecYUVBufferPadded::conditionalDeallocate(paddedSize,bpp,chroma_format);
}
void H264DecYUVWorkSpace::expand()
{
    //IppvcFrameFieldFlag flag;

    if (!isExpanded())
    {
        Ipp32s iWidth = lumaSize().width;
        Ipp32s iHeight = lumaSize().height;

        // expand luma plane
        ippiExpandPlane_H264(m_pYPlane,
                             iWidth,
                             iHeight,
                             pitch(),
                             YUV_Y_PADDING);

        // expand chroma plane(s)
        if (0 < m_chroma_format)
        {
            Ipp32s iChromaWidth = ((m_chroma_format < 3) ? (iWidth >> 1) : (iWidth));
            Ipp32s iChromaHeight = ((m_chroma_format == 1) ? (iHeight >> 1) : (iHeight));
            Ipp32s iYUVPadding = ((m_chroma_format == 3) ? (YUV_UV_PADDING << 1) : (YUV_UV_PADDING));

            ippiExpandPlane_H264(m_pUPlane,
                                 iChromaWidth,
                                 iChromaHeight,
                                 pitch(),
                                 iYUVPadding);

            ippiExpandPlane_H264(m_pVPlane,
                                 iChromaWidth,
                                 iChromaHeight,
                                 pitch(),
                                 iYUVPadding);
        }
    }

} // void H264DecYUVWorkSpace::expand(bool is_field_flag, Ipp8u is_bottom_field)

} // end namespace UMC
