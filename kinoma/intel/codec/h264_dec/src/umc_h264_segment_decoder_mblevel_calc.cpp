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

#include "umc_h264_segment_decoder.h"
#include "umc_h264_dec.h"
#include "vm_debug.h"

namespace UMC
{

Ipp32s H264SegmentDecoder::MBAddr2MBCount(Ipp32s MbAddr)
{
    //***bnie:if (MbAddr<=0) 
	//***bnie:	return MbAddr;

    return MbAddr;

} // Ipp32s H264SegmentDecoder::MBAddr2MBCount(Ipp32s MbAddr)


Ipp32s H264SegmentDecoder::GetColocatedLocation(H264VideoDecoder::H264DecoderFrame *pRefFrame,
                                                Ipp8u Field,
                                                Ipp32s &block,
                                                Ipp8s *scale)
{
    //***bnie: Ipp32u cur_pic_struct=m_pCurrentFrame->m_PictureStructureForDec;
    //***bnie: Ipp32u ref_pic_struct=pRefFrame->m_PictureStructureForDec;
    //Ipp32s xCol=block&3;
    //Ipp32s yCol=block-xCol;
    //Ipp32s mb_width = pRefFrame->macroBlockSize().width;

    //***bnie: if (cur_pic_struct==FRM_STRUCTURE && ref_pic_struct==FRM_STRUCTURE)
    {
        if(scale) *scale=0;
        return m_CurMBAddr;
    }

    VM_ASSERT(0);

    return -1;

} // Ipp32s H264SegmentDecoder::GetColocatedLocation(DecodedFrame *pRefFrame, Ipp8u Field, Ipp32s &block, Ipp8s *scale)

Status H264SegmentDecoder::AdjustIndex(Ipp8u ref_mb_is_bottom, Ipp8s ref_mb_is_field, Ipp8s &RefIdx)
{
    Status ps=UMC_OK;
    if (RefIdx<0)
    {
        RefIdx=0;
        return ps;
    }

    return ps;

} // Status H264SegmentDecoder::AdjustIndex(Ipp8u ref_mb_is_bottom, Ipp8s ref_mb_is_field, Ipp8s &RefIdx)


void H264SegmentDecoder::GetLeftLumaBlockAcrossBoundary(H264DecoderBlockLocation *Block,Ipp8s y)
{
    {
        Block->mb_num = m_cur_mb.CurrentMacroblockNeighbours.mb_A;
        Block->block_num = (y/4)*4+3;
    }

} // void H264SegmentDecoder::GetLeftLumaBlockAcrossBoundary(H264DecoderBlockLocation *Block,Ipp8s y)

void H264SegmentDecoder::GetLeftChromaMacroBlockAcrossBoundary(Ipp32s& mb)
{
    {
        mb = m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    }

} // void H264SegmentDecoder::GetLeftChromaMacroBlockAcrossBoundary(Ipp32s& mb)

void H264SegmentDecoder::ResetGMBData()
{/*
    Ipp32s i,j, firstj = 0;
    mb_width  = m_pCurrentFrame->macroBlockSize().width;
    mb_height = m_pCurrentFrame->macroBlockSize().height;
    Ipp8u mbaff = m_pSliceHeader->MbaffFrameFlag;
    H264DecoderMacroblockGlobalInfo *g_mb_first=m_pCurrentFrame->m_mbinfo.mbs;
    H264DecoderMacroblockGlobalInfo *g_mb;
    if(m_pCurrentFrame->m_PictureStructureForDec<FRM_STRUCTURE)
    {
        mb_height >>= 1;
        if(m_pCurrentFrame->m_bottom_field_flag[m_field_index])
        {
            g_mb_first+= m_pCurrentFrame->totalMBs;
            firstj += mb_height;
        }
    }
    g_mb = g_mb_first;

    for (j = 0; j < mb_height; j++)
    {
        for (i = 0; i < mb_width; i++, g_mb++)
        {
            g_mb->slice_id = -1;
            g_mb->mb_aux_fields = (Ipp8u) (j&mbaff)<<1;
        }
    }*/

} // void H264SegmentDecoder::ResetGMBData()

} // namespace UMC
