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
#ifndef __UMC_H264_DEC_DEFS_YUV_H__
#define __UMC_H264_DEC_DEFS_YUV_H__

#include "umc_h264_dec_defs_dec.h"
#include "umc_video_decoder.h"

namespace UMC
{

struct sDimensions
{
        Ipp32u            width;
        Ipp32u            height;

        sDimensions()                        { width = height = 0; }
        sDimensions(Ipp32u w, Ipp32u h)        { width = w; height = h; }

        // Define equivalence operators for comparing dimensions.

        int operator == (const sDimensions &oprnd) const
        { return width == oprnd.width && height == oprnd.height; };

        int operator != (const sDimensions &oprnd) const
        { return !(*this == oprnd); };
};

//
// Define classes that encapsulate pitched YUV image data.
//


// The H264DecYUVBufferPadded class represents a YUV image laid out in memory such
// that there are padding bytes surrounding the three planes.  This extra
// padding allows pixels at the edge of the planes to be replicated, as
// well as facilitating algorithms that need to read or write a few bytes
// beyond the end of a line of pixels.  The frame is laid out as shown below.
//
//          +---------------------+
//          |                     |
//          |    +-----------+    |     The amount of padding around the
//          |    |           |    |     U and V planes is half the amount
//          |    |     Y     |    |     around the Y plane.  The dotted line
//          |    |           |    |     separating the planes is there just
//          |    +-----------+    |     to emphasize that the padding around
//          |                     |     each plane does not overlap the
//          | .  .  .  .  .  .  . |     padding around the other planes.
//          |          .          |
//          |  +----+  .  +----+  |     Note that all three planes have
//          |  | U  |  .  |  V |  |     the same pitch.
//          |  +----+  .  +----+  |
//          +---------------------+
//
// Note that the class is designed to allow reusing existing buffer space
// for a smaller image size.  For example, if the frame buffer was originally
// allocated to hold a CIF image, and is then adjusted to hold a QCIF image,
// then the QCIF Y, U and V planes will reside in or about the upper left
// quadrant of the CIF-size frame buffer.  Such reuse can result in much
// wasted space.  If this is undesireable, then the existing large buffer
// should be explicitly deallocated prior to allocating the smaller size.
//

// To be fully general, this class should allow the amount of padding to be
// specified via a constructor parameter.  However, we don't need such
// generality currently.  To simplify the class, we will use the hard coded
// padding amounts defined below.

#define YUV_Y_PADDING       32
    // The Y plane has this many bytes (a.k.a. pels) of padding around each
    // of its 4 sides

#define YUV_UV_PADDING      16
    // The U and V planes have this many bytes (a.k.a. pels) of padding
    // around each of their 4 sides

#define YUV_ALIGNMENT       64
    // The beginning of the padded buffer is aligned thusly.

class H264DecYUVBufferPadded
{
    DYNAMIC_CAST_DECL_BASE(H264DecYUVBufferPadded)
public:
        Ipp8u                 *m_pAllocatedBuffer;
            // m_pAllocatedBuffer contains the pointer returned when
            // we allocated space for the data.

        Ipp32u                 m_allocatedSize;
            // This is the size with which m_pAllocatedBuffer was allocated.

        Ipp8u                 *m_pBuffer;
            // m_pBuffer is a "YUV_ALIGNMENT"-byte aligned address, pointing
            // to the beginning of the padded YUV data within
            // m_pAllocatedBuffer.

        sDimensions                m_lumaSize;
            // m_lumaSize specifies the dimensions of the Y plane, as
            // specified when allocate() was most recently called.
            //
            // For clarity, it should be noted that in the context of our
            // codec, these dimensions have typically already been rounded
            // up to a multiple of 16.  However, such rounding is performed
            // outside of this class.  We use whatever dimensions come into
            // allocate().

        Ipp32u                    m_pitch;
        Ipp8u                     m_bpp;
        Ipp8u                     m_chroma_format;
            // m_pitch is 0 if the buffer hasn't been allocated.
            // Otherwise it is the current pitch in effect (typically
            // 2*Y_PADDING + m_lumaSize.width).

public:
            Ipp8u                *m_pYPlane;
            Ipp8u                *m_pUPlane;
            Ipp8u                *m_pVPlane;
            Ipp16u                *m_pYPlaneAdv;
            Ipp16u                *m_pUPlaneAdv;
            Ipp16u                *m_pVPlaneAdv;


                    H264DecYUVBufferPadded();
virtual            ~H264DecYUVBufferPadded();

            void    clear()
            {
                m_pYPlane = m_pUPlane = m_pVPlane = 0;
                m_pYPlaneAdv = m_pUPlaneAdv = m_pVPlaneAdv = 0;
            }

virtual        Status                    allocate(const sDimensions &lumaSize,Ipp8u bpp,Ipp8u chroma_format);
            // Reallocate the buffer, if necessary, so that it is large enough
            // to hold a YUV image of the given dimensions, and set the plane
            // pointers and pitch appropriately.  If the existing buffer is
            // already big enough, then we reuse it.  If this behavior is not
            // desired, then call deallocate() prior to calling allocate().

virtual        void                    deallocate();
            // Deallocate the buffer and clear all state information.

virtual        void                    conditionalDeallocate(const sDimensions&,Ipp8u bpp,Ipp8u chroma_format);
            // Deallocate the buffer only if its current luma dimensions
            // differ from those specified in the parameter.

            const sDimensions&        lumaSize() { return m_lumaSize; }

            Ipp32u                   pitch()    { return m_pitch; }
};


// The H264DecYUVWorkSpace class represents a padded YUV image which can have
// various processing performed on it, such as replicating the pixels
// around its edges, and applying postfilters.

class H264DecYUVWorkSpace : public H264DecYUVBufferPadded
{
private:

        bool            m_isExpanded;
            // This indicates whether pixels around the edges of the planes
            // have been replicated into the surrounding padding area.
            // The expand() method performs this operation.

        sDimensions        m_macroBlockSize;
        //sDimensions        m_subBlockSize;
            // The image's width and height, in units of macroblocks and
            // 4x4 subblocks.  For example, a QCIF image is 11 MBs wide and
            // 9 MBs high; or 44 subblocks wide and 36 subblocks high.

private:
        void        clearState()
        {
            // Reset our state to indicate no processing has been performed.
            // This is called when our buffer is allocated.
            m_isExpanded = false;
            m_macroBlockSize /*= m_subBlockSize*/ = sDimensions(0,0);
        }

public:
                    H264DecYUVWorkSpace() { clearState(); }
virtual            ~H264DecYUVWorkSpace() { }

virtual Status                allocate(const sDimensions &lumaSize,Ipp8u bpp,Ipp8u chroma_format);
            // Reallocate the buffer, if necessary, so that it is large enough
            // to hold a YUV image of the given dimensions, rounded up to
            // the next macroblock boundary.  If the existing buffer is
            // already big enough, then we reuse it.  If this behavior is not
            // desired, then call deallocate() prior to calling allocate().
            // Regardless of whether the buffer is reallocated, our state
            // is cleared to indicate that no processing has been performed
            // on this buffer.

virtual void                conditionalDeallocate(const sDimensions &,Ipp8u,Ipp8u);
            // Deallocate the buffer only if its current dimensions
            // differ from those specified in the parameter, after rounding
            // up to the next macroblock boundary.

        void                expand();

        bool                isExpanded()     { return m_isExpanded; }
        void                setExpanded()     { m_isExpanded = true; }

        const sDimensions&    macroBlockSize() { return m_macroBlockSize; }
        //const sDimensions&    subBlockSize()   { return m_subBlockSize; }


        void                copyState(H264DecYUVWorkSpace &/*src*/)
        {
            m_isExpanded  = false;
        }


};

} // end namespace UMC

#endif // __UMC_H264_DEC_DEFS_YUV_H__
