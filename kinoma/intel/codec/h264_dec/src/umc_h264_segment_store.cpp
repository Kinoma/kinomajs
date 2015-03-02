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

#include "umc_h264_dec_defs_dec.h"
#include "umc_structures.h"

namespace UMC
{

H264DecoderLocalMacroblockDescriptor::H264DecoderLocalMacroblockDescriptor(void)
{
    MVDeltas[0] =
    MVDeltas[1] = NULL;
    MVFlags[0] =
    MVFlags[1] = NULL;
    MacroblockCoeffsInfo = NULL;
    mbs = NULL;
    active_next_mb_table = NULL;

    m_pAllocated = NULL;
    m_nAllocatedSize = 0;

} // H264DecoderLocalMacroblockDescriptor::H264DecoderLocalMacroblockDescriptor(void)

H264DecoderLocalMacroblockDescriptor::~H264DecoderLocalMacroblockDescriptor(void)
{
    Release();

} // H264DecoderLocalMacroblockDescriptor::~H264DecoderLocalMacroblockDescriptor(void)

void H264DecoderLocalMacroblockDescriptor::Release(void)
{
    if (m_pAllocated)
        ippsFree_x(m_pAllocated);

    MVDeltas[0] =
    MVDeltas[1] = NULL;
    MVFlags[0] =
    MVFlags[1] = NULL;
    MacroblockCoeffsInfo = NULL;
    mbs = NULL;
    active_next_mb_table = NULL;

    m_pAllocated = NULL;
    m_nAllocatedSize = 0;

} // void H264DecoderLocalMacroblockDescriptor::Release(void)

bool H264DecoderLocalMacroblockDescriptor::Allocate(Ipp32s iMBCount)
{
    // allocate buffer
    size_t nSize = (sizeof(H264DecoderMacroblockMVs) +
                    sizeof(H264DecoderMacroblockMVs) +
                    sizeof(H264DecoderMacroblockMVFlags) +
                    sizeof(H264DecoderMacroblockMVFlags) +
                    sizeof(H264DecoderMacroblockCoeffsInfo) +
                    sizeof(H264DecoderMacroblockLocalInfo)) * iMBCount + 16 * 6;

    if ((NULL == m_pAllocated) ||
        (m_nAllocatedSize < nSize))
    {
        // release object before initialization
        Release();

        m_pAllocated = ippsMalloc_8u_x(nSize);
        if (NULL == m_pAllocated)
            return false;
        ippsZero_8u_x(m_pAllocated, nSize);
        m_nAllocatedSize = nSize;
    }

    // reset pointer(s)
    MVDeltas[0] = align_pointer<H264DecoderMacroblockMVs *> (m_pAllocated, ALIGN_VALUE);
    MVDeltas[1] = align_pointer<H264DecoderMacroblockMVs *> (MVDeltas[0] + iMBCount, ALIGN_VALUE);
    MVFlags[0] = align_pointer<H264DecoderMacroblockMVFlags *> (MVDeltas[1] + iMBCount, ALIGN_VALUE);
    MVFlags[1] = align_pointer<H264DecoderMacroblockMVFlags *> (MVFlags[0] + iMBCount, ALIGN_VALUE);
    MacroblockCoeffsInfo = align_pointer<H264DecoderMacroblockCoeffsInfo *> (MVFlags[1] + iMBCount, ALIGN_VALUE);
    mbs = align_pointer<H264DecoderMacroblockLocalInfo *> (MacroblockCoeffsInfo + iMBCount, ALIGN_VALUE);

    memset(MacroblockCoeffsInfo, 0, iMBCount * sizeof(H264DecoderMacroblockCoeffsInfo));

    return true;

} // bool H264DecoderLocalMacroblockDescriptor::Allocate(Ipp32s iMBCount)

H264DecoderLocalMacroblockDescriptor &H264DecoderLocalMacroblockDescriptor::operator = (H264DecoderLocalMacroblockDescriptor &Desc)
{
    MVDeltas[0] = Desc.MVDeltas[0];
    MVDeltas[1] = Desc.MVDeltas[1];
    MVFlags[0] = Desc.MVFlags[0];
    MVFlags[1] = Desc.MVFlags[1];
    MacroblockCoeffsInfo = Desc.MacroblockCoeffsInfo;
    mbs = Desc.mbs;
    active_next_mb_table = Desc.active_next_mb_table;

    return *this;

} // H264DecoderLocalMacroblockDescriptor &H264DecoderLocalMacroblockDescriptor::operator = (H264DecoderLocalMacroblockDescriptor &Dest)

} // namespace UMC
