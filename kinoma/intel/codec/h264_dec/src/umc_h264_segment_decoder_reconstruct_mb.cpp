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
#include "umc_h264_dec_ipplevel.h"

namespace UMC
{

    void H264SegmentDecoder::ReconstructMacroblockBMEHC(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1)
    {
        Ipp8u   *pRefYPlane;
        Ipp8u   *pRefVPlane;
        Ipp8u   *pRefUPlane;
        Ipp8u   *pRef;
        H264DecoderMotionVector *pMV;

        Ipp32u mbtype = m_cur_mb.GlobalMacroblockInfo->mbtype;

        Ipp8u *psbdir = m_cur_mb.LocalMacroblockInfo->sbdir;
        Ipp32s offsetY;
        Ipp32s offsetC;
        Ipp32s pitch;
        Ipp32u block, i, loopCnt;
        Ipp32s xint, yint, xh, yh;
        Ipp32s mvx, mvy, mvyc;
        Ipp32s width;
        Ipp32s height;
        Ipp32s xpos, ypos;
        IppiSize        roi = {-1, -1}, roi_cr = {0};

        // pointers for current subblock
        H264DecoderMotionVector *pMV_sb;
        Ipp8u *pRefY_sb;
        Ipp8u *pDstY_sb = NULL;
        Ipp8u *pRefV_sb;
        Ipp8u *pDstV_sb = NULL;
        Ipp8u *pRefU_sb;
        Ipp8u *pDstU_sb = NULL;
        Ipp8u *pTmpY = NULL;
        Ipp8u *pTmpU = NULL;
        Ipp8u *pTmpV = NULL;
        Ipp32s nTmpPitch = 16;

        Ipp32u uBlockDir;        // one of D_DIR_FWD, D_DIR_BWD, D_DIR_BIDIR
        bool bBidirWeightMB = false;    // is bidir weighting in effect for the MB?
        bool bUnidirWeightMB = false;    // is explicit L0 weighting in effect for the MB?
        bool bUnidirWeightSB = false;    // is explicit L0 weighting in effect for the subblock?

        bool is_need_check_expand = !m_CurMB_Y || m_CurMB_Y > mb_height - 2;

        // Optional weighting vars
        Ipp32u CurrPicParamSetId;
        Ipp32u weighted_bipred_idc = 0;
        Ipp32u luma_log2_weight_denom = 0;
        Ipp32u chroma_log2_weight_denom = 0;
        const Ipp32s *pDistScaleFactors = NULL;

        H264DecoderMotionVector *pMVFwd = NULL;
        H264DecoderMotionVector *pMVBwd = NULL;
        Ipp8s *pRefIndexL0 = NULL;
        Ipp8s *pRefIndexL1 = NULL;
        Ipp8s RefIndexL0 = 0;
        Ipp8s RefIndexL1 = 0;
        Ipp8s RefIndex = 0;

        Ipp32s temp_offset;

        VM_ASSERT(IS_INTER_MBTYPE(m_cur_mb.GlobalMacroblockInfo->mbtype));

        width = m_pCurrentFrame->lumaSize().width;
        height = m_pCurrentFrame->lumaSize().height;
        pitch = m_pCurrentFrame->pitch();

        offsetY = mbXOffset + mbYOffset*pitch;
        offsetC = (mbXOffset >> 1) +  ((mbYOffset>> 1) * pitch);
        CurrPicParamSetId = m_pSliceHeader->pic_parameter_set_id;

        pMVFwd = m_cur_mb.MVs[0]->MotionVectors;
        pRefIndexL0 = m_cur_mb.RefIdxs[0]->RefIdxs;

        if (((PREDSLICE == m_pSliceHeader->slice_type) ||
            (S_PREDSLICE == m_pSliceHeader->slice_type)) &&
            (m_pPicParamSet[CurrPicParamSetId].weighted_pred_flag != 0))
        {
            // L0 weighting specified in pic param set. Get weighting params
            // for the slice.
            luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
            chroma_log2_weight_denom = m_pSliceHeader->chroma_log2_weight_denom;
            bUnidirWeightMB = true;
        }

        // get luma interp func pointer table in cache
        if (m_pSliceHeader->slice_type == BPREDSLICE)
        {
            VM_ASSERT(pRefPicList1[0]);

            pMVBwd = m_cur_mb.MVs[1]->MotionVectors;
            pRefIndexL1 = m_cur_mb.RefIdxs[1]->RefIdxs;
            // DIRECT MB have the same subblock partition structure as the
            // colocated MB. Take advantage of that to perform motion comp
            // for the direct MB using the largest partitions possible.
            if (mbtype == MBTYPE_DIRECT || mbtype == MBTYPE_SKIPPED)
            {
                mbtype = MBTYPE_INTER_8x8;
            }

            // Bi-dir weighting?
            weighted_bipred_idc = m_pPicParamSet[CurrPicParamSetId].weighted_bipred_idc;
            if (weighted_bipred_idc == 1)
            {
                // explicit bidir weighting
                luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
                chroma_log2_weight_denom = m_pSliceHeader->chroma_log2_weight_denom;
                bUnidirWeightMB = true;
                bBidirWeightMB = true;
            }
            if (weighted_bipred_idc == 2)
            {
                pDistScaleFactors = m_pSlice->GetDistScaleFactor();
                bBidirWeightMB = true;
            }
        }

        if (mbtype != MBTYPE_INTER_8x8 && mbtype != MBTYPE_INTER_8x8_REF0)
        {
            if (mbtype == MBTYPE_INTER_16x8)
            {
                roi.width        = 16;
                roi.height       = 8;
                roi_cr.width     = 16>>1;
                roi_cr.height    = 8>>1;
				this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_16x8;
            }
            else if (mbtype == MBTYPE_INTER_8x16)
            {
                roi.width        = 8;
                roi.height       = 16;
                roi_cr.width     = 8>>1;
                roi_cr.height    = 16>>1;
				this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x16;
            }
            else
            {
                roi.width          = 16;
                roi.height         = 16;
                roi_cr.width       = 16>>1;
                roi_cr.height      = 16>>1;
				this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_16x16;
            }
            VM_ASSERT(0 < (signed) roi.width);
            VM_ASSERT(0 < (signed) roi.height);
            block = 0;
            for (ypos=0; ypos<16; ypos+=roi.height)
            {
                for (xpos=0; xpos<16; xpos+=roi.width)
                {
                    if ((mbtype == MBTYPE_BIDIR) || (psbdir[block] == D_DIR_BIDIR))
                    {
                        uBlockDir = D_DIR_BIDIR;
                        loopCnt = 2;
                        bUnidirWeightSB = false;
                    }
                    else
                    {
                        loopCnt = 1;
                        if ((mbtype == MBTYPE_BACKWARD) ||
                            (psbdir[block] == D_DIR_BWD))
                        {
                            uBlockDir = D_DIR_BWD;
                        } else
                            uBlockDir = D_DIR_FWD;
                        bUnidirWeightSB =  bUnidirWeightMB;
                    }

                    for (i = 0; i<loopCnt; i++)
                    {
                        if (uBlockDir == D_DIR_BWD || i)
                        {
                            RefIndexL1 = RefIndex = pRefIndexL1[(xpos>>2) + (ypos>>2)*4];
                            VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

 							//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie: 
							//	return;
							//}

							pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane + offsetY;
                            pRefVPlane = pRefPicList1[RefIndexL1]->m_pVPlane + offsetC;
                            pRefUPlane = pRefPicList1[RefIndexL1]->m_pUPlane + offsetC;

                            VM_ASSERT(pRefYPlane);
                            VM_ASSERT(pRefVPlane);
                            VM_ASSERT(pRefUPlane);

                            pMV = pMVBwd;
                        }
                        else
                        {
                            RefIndexL0 = RefIndex = pRefIndexL0[(xpos>>2) + (ypos>>2)*4];

                            VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

							//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie:
							//	return;
							//}

                            pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane + offsetY;
                            pRefVPlane = pRefPicList0[RefIndexL0]->m_pVPlane + offsetC;
                            pRefUPlane = pRefPicList0[RefIndexL0]->m_pUPlane + offsetC;

                            VM_ASSERT(pRefYPlane);
                            VM_ASSERT(pRefVPlane);
                            VM_ASSERT(pRefUPlane);

                            pMV = pMVFwd;
                        }
                        // set pointers for this subblock
                        pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;
                        mvx = pMV_sb->mvx;
                        mvy = pMV_sb->mvy;

                        temp_offset = xpos + ypos*pitch;
                        pDstY_sb = pDstY + temp_offset;
                        pRefY_sb = pRefYPlane + temp_offset;
                        pDstV_sb = pDstV + (temp_offset >> 1);
                        pRefV_sb = pRefVPlane + (temp_offset >> 1);
                        pDstU_sb = pDstU + (temp_offset >> 1);
                        pRefU_sb = pRefUPlane + (temp_offset >> 1);

                        if (i > 0)
                        {
                            pTmpY = m_pPredictionBuffer;
                            pTmpU = pTmpY + 16 * 16;
                            pTmpV = pTmpU + 16 * 16;
                            nTmpPitch = 16;
                        }
                        else
                        {
                            pTmpY = pDstY_sb;
                            pTmpU = pDstU_sb;
                            pTmpV = pDstV_sb;
                            nTmpPitch = pitch;
                        }

                        xh = mvx & (INTERP_FACTOR-1);
                        yh = mvy & (INTERP_FACTOR-1);

                        // Note must select filter (get xh, yh) before clipping in
                        // order to preserve selection. Makes a difference only
                        // when 3,3 filter is selected. Now clip mvx and mvy
                        // rather than xint and yint to avoid clipping again for
                        // chroma.

                        Ipp8u pred_method = 0;
                        if (ABS(mvy) < (13 << INTERP_SHIFT))
                        {
                            if (is_need_check_expand)
                            {
                                pred_method = SelectPredictionMethod(
                                    mbYOffset+ypos,
                                    mvy,
                                    roi.height,
                                    height);
                            }
                        } else {
                            pred_method = SelectPredictionMethod(
                                mbYOffset+ypos,
                                mvy,
                                roi.height,
                                height);

                            mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                                1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                        }

                        if (ABS(mvx) > (D_MV_CLIP_LIMIT << INTERP_SHIFT))
                        {
                            mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width -
                                1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                        }

                        mvyc = mvy;

                        xint = mvx >> INTERP_SHIFT;
                        yint = mvy >> INTERP_SHIFT;

                        pRef = pRefY_sb + xint + yint * pitch;
                        switch(pred_method)
                        {
                        case ALLOK:
                            this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                pTmpY, nTmpPitch,
                                xh, yh, roi);
                            break;
                        case PREDICTION_FROM_TOP:
                            ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpY, nTmpPitch,
                                xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                            break;
                        case PREDICTION_FROM_BOTTOM:
                            ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpY, nTmpPitch,
                                xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                            break;

                        default:VM_ASSERT(0);
                            break;
                        }

                        // optional prediction weighting
                        if (bUnidirWeightSB &&
                            m_pPredWeight[uBlockDir][RefIndex].luma_weight_flag != 0)
                        {
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpY, nTmpPitch,
                                luma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][RefIndex].luma_weight,
                                m_pPredWeight[uBlockDir][RefIndex].luma_offset,
                                roi);
                        }
                        // chroma (1/8 pixel MV)
                        xh = (mvx &  (INTERP_FACTOR*2-1));
                        yh = (mvyc & (INTERP_FACTOR*2-1));
                        xint = mvx >>  (INTERP_SHIFT+1);
                        yint = mvyc >> (INTERP_SHIFT+1);

                        temp_offset = xint + yint * pitch;
                        switch(pred_method)
                        {
                        case ALLOK:
                            pRef = pRefV_sb + temp_offset;

                            ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                pTmpV, nTmpPitch,
                                xh, yh,   roi_cr);

                            pRef = pRefU_sb + temp_offset;

                            ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                pTmpU, nTmpPitch,
                                xh, yh,   roi_cr);
                            break;

                        case PREDICTION_FROM_TOP:
                            pRef = pRefV_sb + temp_offset;

                            ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpV, nTmpPitch,
                                xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>> 1)+yint), roi_cr);


                            pRef = pRefU_sb + temp_offset;

                            ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpU, nTmpPitch,
                                xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>> 1)+yint), roi_cr);

                            break;
                        case PREDICTION_FROM_BOTTOM:
                            pRef = pRefV_sb + temp_offset;

                            ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpV, nTmpPitch,
                                xh, yh,   ((((Ipp32s)mbYOffset+ypos)>>1)+yint+roi_cr.height)-(height>>1),roi_cr);

                            pRef = pRefU_sb + temp_offset;

                            ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpU, nTmpPitch,
                                xh, yh,   ((((Ipp32s)mbYOffset+ypos)>>1)+yint+roi_cr.height)-(height>>1),roi_cr);
                            break;
                        default:VM_ASSERT(0);
                            break;
                        }
                        // optional prediction weighting
                        if (bUnidirWeightSB &&
                            m_pPredWeight[uBlockDir][RefIndex].chroma_weight_flag != 0)
                        {
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpV,
                                nTmpPitch,
                                chroma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][RefIndex].chroma_weight[1],
                                m_pPredWeight[uBlockDir][RefIndex].chroma_offset[1],
                                roi_cr);
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpU,
                                nTmpPitch,
                                chroma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][RefIndex].chroma_weight[0],
                                m_pPredWeight[uBlockDir][RefIndex].chroma_offset[0],
                                roi_cr);
                        }
                    }    // loopCnt
                    if (loopCnt > 1)
                    {
                        if (!bBidirWeightMB)
                        {
                            // combine bidir predictions into one, no weighting
                            // luma

                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb,
                                pTmpY,
                                pDstY_sb,
                                roi.width,
                                roi.height,
                                pitch,
                                nTmpPitch,
                                pitch);

                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstV_sb,
                                pTmpV,
                                pDstV_sb,
                                roi_cr.width,
                                roi_cr.height,
                                pitch,
                                nTmpPitch,
                                pitch);
                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstU_sb,
                                pTmpU,
                                pDstU_sb,
                                roi_cr.width,
                                roi_cr.height,
                                pitch,
                                nTmpPitch,
                                pitch);
                        }
                        else
                        {
                            // combine bidir predictions into one with weighting
                            if (weighted_bipred_idc == 1)
                            {
                                // combine bidir predictions into one, explicit weighting

                                // luma
                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb,
                                    pTmpY,
                                    pDstY_sb,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    luma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].luma_weight,
                                    m_pPredWeight[0][RefIndexL0].luma_offset,
                                    m_pPredWeight[1][RefIndexL1].luma_weight,
                                    m_pPredWeight[1][RefIndexL1].luma_offset,
                                    roi);
                                // chroma
                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstV_sb,
                                    pTmpV,
                                    pDstV_sb,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                    m_pPredWeight[0][RefIndexL0].chroma_offset[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[1],
                                    roi_cr);

                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstU_sb,
                                    pTmpU,
                                    pDstU_sb,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                    m_pPredWeight[0][RefIndexL0].chroma_offset[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[0],
                                    roi_cr);
                            }
                            else if (weighted_bipred_idc == 2)
                            {
                                // combine bidir predictions into one, implicit weighting
                                Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;

                                // luma
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb,
                                    pTmpY,
                                    pDstY_sb,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi);
                                // chroma
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstV_sb,
                                    pTmpV,
                                    pDstV_sb,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi_cr);
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstU_sb,
                                    pTmpU,
                                    pDstU_sb,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi_cr);
                            }
                            else
                                VM_ASSERT(0);
                        }    // weighted
                    }    // LoopCnt > 1
                    block++;
                }    // xpos
            }    // ypos
        }    // not 8x8
        else
        {
            IppiSize roi1 = {8, 8}, roi_cr1 = {4, 4};
            Ipp8u *psbtype = m_cur_mb.GlobalMacroblockInfo->sbtype;

            // MBTYPE_INTER_8x8:
            // 4 8x8 subblocks, each of which can be a different mode.
            for (block=0; block<4; block++)
            {
                switch (psbtype[block])
                {
                case SBTYPE_8x8:
                    roi.width        = 8;
                    roi.height       = 8;
                    roi_cr.width     = 8>>1;
                    roi_cr.height    = 8>>1;
					this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x8;
					break;
                case SBTYPE_8x4:
                    roi.width        = 8;
                    roi.height       = 4;
                    roi_cr.width     = 8>>1;
                    roi_cr.height    = 4>>1;
					this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x4;
                    break;
                case SBTYPE_4x8:
                    roi.width        = 4;
                    roi.height       = 8;
                    roi_cr.width     = 4>>1;
                    roi_cr.height    = 8>>1;
					this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_4x8;
                    break;
                case SBTYPE_DIRECT:        // for spatial mode DIRECT
                    roi.width        = 4<<(Ipp8u)bUseDirect8x8Inference;
                    roi.height       = 4<<(Ipp8u)bUseDirect8x8Inference;
                    roi_cr.width     = (4<<(Ipp8u)bUseDirect8x8Inference)>>1;
                    roi_cr.height    = (4<<(Ipp8u)bUseDirect8x8Inference)>>1;
					if( bUseDirect8x8Inference )
						this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x8;//***generic  bnie 2/3/2009 
					else
						this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_4x4;//***generic  bnie 2/3/2009 
					break;
                case SBTYPE_4x4:
                    roi.width        = 4;
                    roi.height       = 4;
                    roi_cr.width     = 4>>1;
                    roi_cr.height    = 4>>1;
					this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_4x4;
                    break;
                default:
                    VM_ASSERT(0);
                    return;
                }    // switch sbtype

                if ((psbdir[block] == D_DIR_BIDIR) ||
                    (psbdir[block] == D_DIR_DIRECT) ||
                    (psbdir[block] == D_DIR_DIRECT_SPATIAL_BIDIR))
                {
                    uBlockDir = D_DIR_BIDIR;
                    loopCnt = 2;
                    bUnidirWeightSB =  false;
                }
                else
                {
                    loopCnt = 1;
                    if ((psbdir[block] == D_DIR_BWD) ||
                        (psbdir[block] == D_DIR_DIRECT_SPATIAL_BWD))
                    {
                        uBlockDir = D_DIR_BWD;
                    } else {
                        uBlockDir = D_DIR_FWD;
                    }
                    bUnidirWeightSB =  bUnidirWeightMB;
                }

                for (i = 0; i<loopCnt; i++)
                {
                    ypos = yoff8[block];
                    xpos = xoff8[block];

                    if (uBlockDir == D_DIR_BWD || i)
                    {
                        RefIndexL1 = RefIndex = pRefIndexL1[(xpos>>2) + (ypos>>2)*4];

                        VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                            (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

						//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
						//{//***bnie: 
						//	return;
						//}

                        pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane + offsetY;
                        pRefVPlane = pRefPicList1[RefIndexL1]->m_pVPlane + offsetC;
                        pRefUPlane = pRefPicList1[RefIndexL1]->m_pUPlane + offsetC;

                        VM_ASSERT(pRefYPlane);
                        VM_ASSERT(pRefVPlane);
                        VM_ASSERT(pRefUPlane);

                        pMV = pMVBwd;
                    }
                    else
                    {
                        RefIndexL0 = RefIndex = pRefIndexL0[(xpos>>2) + (ypos>>2)*4];

                        VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                            (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

						//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
						//{//***bnie: 
						//	return;
						//}

                        pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane + offsetY;
                        pRefVPlane = pRefPicList0[RefIndexL0]->m_pVPlane + offsetC;
                        pRefUPlane = pRefPicList0[RefIndexL0]->m_pUPlane + offsetC;

                        VM_ASSERT(pRefYPlane);
                        VM_ASSERT(pRefVPlane);
                        VM_ASSERT(pRefUPlane);

                        pMV = pMVFwd;
                    }

                    for (; ypos < yoff8[block] + 8; ypos+=roi.height)
                    {
                        for (xpos = xoff8[block]; xpos < xoff8[block] + 8; xpos+=roi.width)
                        {
                            temp_offset = xpos + ypos*pitch;
                            pDstY_sb = pDstY + temp_offset;
                            pRefY_sb = pRefYPlane + temp_offset;
                            pDstV_sb = pDstV + (temp_offset >> 1);
                            pRefV_sb = pRefVPlane + (temp_offset >> 1);
                            pDstU_sb = pDstU + (temp_offset >> 1);
                            pRefU_sb = pRefUPlane + (temp_offset >> 1);

                            if (i > 0)
                            {
                                pTmpY = m_pPredictionBuffer;
                                pTmpU = pTmpY + 16 * 16;
                                pTmpV = pTmpU + 16 * 16;

                                nTmpPitch = 16;
                                temp_offset = xpos + ypos*nTmpPitch;
                                pTmpY += temp_offset;
                                pTmpU += temp_offset >> 1;
                                pTmpV += temp_offset >> 1;
                            }
                            else
                            {
                                pTmpY = pDstY_sb;
                                pTmpU = pDstU_sb;
                                pTmpV = pDstV_sb;
                                nTmpPitch = pitch;
                            }

                            // set pointers for this subblock
                            pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;

                            mvx = pMV_sb->mvx;
                            mvy = pMV_sb->mvy;

                            xh = mvx & (INTERP_FACTOR-1);
                            yh = mvy & (INTERP_FACTOR-1);

                            Ipp8u pred_method = 0;
                            if (ABS(mvy) < (13 << INTERP_SHIFT))
                            {
                                if (is_need_check_expand)
                                {
                                    pred_method = SelectPredictionMethod(
                                        mbYOffset+ypos,
                                        mvy,
                                        roi.height,
                                        height);
                                }
                            } else {
                                pred_method = SelectPredictionMethod(
                                    mbYOffset+ypos,
                                    mvy,
                                    roi.height,
                                    height);

                                mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                                    1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                                mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                            }

                            if (ABS(mvx) > (D_MV_CLIP_LIMIT << INTERP_SHIFT))
                            {
                                // See comment above about clipping & xh,yh.
                                mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width - 1
                                    - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                                mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                            }

                            mvyc = mvy;

                            xint = mvx >> INTERP_SHIFT;
                            yint = mvy >> INTERP_SHIFT;

                            pRef = pRefY_sb + xint + yint * pitch;
                            switch(pred_method)
                            {
                            case ALLOK:
                                this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, roi);

                                break;
                            case PREDICTION_FROM_TOP:
                                ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                                break;
                            case PREDICTION_FROM_BOTTOM:
                                ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                                break;

                            default:VM_ASSERT(0);
                                break;
                            }

                            // chroma (1/8 pixel MV)
                            xh = (mvx &  (INTERP_FACTOR*2)-1);
                            yh = (mvyc & (INTERP_FACTOR*2)-1);
                            xint = mvx >>  (INTERP_SHIFT+1);
                            yint = mvyc >> (INTERP_SHIFT+1);

                            temp_offset = xint + yint * pitch;
                            switch(pred_method)
                            {
                            case ALLOK:
                                pRef = pRefV_sb + temp_offset;

                                ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   roi_cr);

                                pRef = pRefU_sb + temp_offset;

                                ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   roi_cr);
                                break;

                            case PREDICTION_FROM_TOP:
                                pRef = pRefV_sb + temp_offset;


                                ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>>1)+yint), roi_cr);


                                pRef = pRefU_sb + temp_offset;

                                ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>>1)+yint), roi_cr);

                                break;
                            case PREDICTION_FROM_BOTTOM:
                                pRef = pRefV_sb + temp_offset;

                                ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   ((((Ipp32s)mbYOffset+ypos)>>1)+yint+roi_cr.height)-(height>>1),roi_cr);

                                pRef = pRefU_sb + temp_offset;

                                ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   ((((Ipp32s)mbYOffset+ypos)>>1)+yint+roi_cr.height)-(height>>1),roi_cr);
                                break;
                            default:VM_ASSERT(0);
                                break;
                            }
                        }    // for xpos
                    }    // for ypos
                }    // loopCnt

                pTmpY = m_pPredictionBuffer;
                pTmpU = pTmpY + 16 * 16;
                pTmpV = pTmpU + 16 * 16;
                nTmpPitch = 16;

                temp_offset = xoff8[block] + yoff8[block]*nTmpPitch;
                pTmpY += temp_offset;
                pTmpU += temp_offset >> 1;
                pTmpV += temp_offset >> 1;

                temp_offset = xoff8[block] + yoff8[block]*pitch;
                pDstY_sb = pDstY + temp_offset;
                pDstV_sb = pDstV + (temp_offset >> 1);
                pDstU_sb = pDstU + (temp_offset >> 1);

                // optional prediction weighting
                if (bUnidirWeightSB &&
                    m_pPredWeight[uBlockDir][RefIndex].luma_weight_flag != 0)
                {
                    ippiUniDirWeightBlock_H264_8u_C1R_x(pDstY_sb,
                        pitch,
                        luma_log2_weight_denom,
                        m_pPredWeight[uBlockDir][RefIndex].luma_weight,
                        m_pPredWeight[uBlockDir][RefIndex].luma_offset,
                        roi1);
                }

                // optional prediction weighting
                if (bUnidirWeightSB &&
                    m_pPredWeight[uBlockDir][RefIndex].chroma_weight_flag != 0)
                {
                    ippiUniDirWeightBlock_H264_8u_C1R_x(pDstV_sb,
                        pitch,
                        chroma_log2_weight_denom,
                        m_pPredWeight[uBlockDir][RefIndex].chroma_weight[1],
                        m_pPredWeight[uBlockDir][RefIndex].chroma_offset[1],
                        roi_cr1);
                    ippiUniDirWeightBlock_H264_8u_C1R_x(pDstU_sb,
                        pitch,
                        chroma_log2_weight_denom,
                        m_pPredWeight[uBlockDir][RefIndex].chroma_weight[0],
                        m_pPredWeight[uBlockDir][RefIndex].chroma_offset[0],
                        roi_cr1);
                }

                if (loopCnt > 1)
                {
                    if (!bBidirWeightMB)
                    {
                        // combine bidir predictions into one, no weighting
                        ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb,
                            pTmpY,
                            pDstY_sb,
                            roi1.width,
                            roi1.height,
                            pitch,
                            nTmpPitch,
                            pitch);
                        ippiInterpolateBlock_H264_8u_P3P1R_x(pDstV_sb,
                            pTmpV,
                            pDstV_sb,
                            roi_cr1.width,
                            roi_cr1.height,
                            pitch,
                            nTmpPitch,
                            pitch);
                        ippiInterpolateBlock_H264_8u_P3P1R_x(pDstU_sb,
                            pTmpU,
                            pDstU_sb,
                            roi_cr1.width,
                            roi_cr1.height,
                            pitch,
                            nTmpPitch,
                            pitch);

                    }
                    else
                    {
                        // combine bidir predictions into one with weighting
                        // combine bidir predictions into one with weighting
                        if (weighted_bipred_idc == 1)
                        {
                            // combine bidir predictions into one, explicit weighting

                            // luma
                            ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb,
                                pTmpY,
                                pDstY_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                luma_log2_weight_denom,
                                m_pPredWeight[0][RefIndexL0].luma_weight,
                                m_pPredWeight[0][RefIndexL0].luma_offset,
                                m_pPredWeight[1][RefIndexL1].luma_weight,
                                m_pPredWeight[1][RefIndexL1].luma_offset,
                                roi1);
                            // chroma
                            ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstV_sb,
                                pTmpV,
                                pDstV_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                chroma_log2_weight_denom,
                                m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                m_pPredWeight[0][RefIndexL0].chroma_offset[1],
                                m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                m_pPredWeight[1][RefIndexL1].chroma_offset[1],
                                roi_cr1);
                            ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstU_sb,
                                pTmpU,
                                pDstU_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                chroma_log2_weight_denom,
                                m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                m_pPredWeight[0][RefIndexL0].chroma_offset[0],
                                m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                m_pPredWeight[1][RefIndexL1].chroma_offset[0],
                                roi_cr1);
                        }
                        else if (weighted_bipred_idc == 2)
                        {
                            // combine bidir predictions into one, implicit weighting
                            Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;

                            // luma
                            ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb,
                                pTmpY,
                                pDstY_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                64 - iDistScaleFactor,
                                iDistScaleFactor,
                                roi1);
                            // chroma
                            ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstV_sb,
                                pTmpV,
                                pDstV_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                64 - iDistScaleFactor,
                                iDistScaleFactor,
                                roi_cr1);
                            ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstU_sb,
                                pTmpU,
                                pDstU_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                64 - iDistScaleFactor,
                                iDistScaleFactor,
                                roi_cr1);
                        }
                        else
                            VM_ASSERT(0);
                    }    //  weighted
                }    // LoopCnt >1
            }    // for block
        }    // 8x8

    } // void H264SegmentDecoder::ReconstructMacroblock(Ipp8u *pDstY,

    void H264SegmentDecoder::ReconstructMacroblockHM(Ipp8u *pDstY,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1)
    {
        Ipp8u   *pRefYPlane;
        Ipp8u   *pRef;
        H264DecoderMotionVector *pMV;
        Ipp32u mbtype = m_cur_mb.GlobalMacroblockInfo->mbtype;

        Ipp8u *psbdir = m_cur_mb.LocalMacroblockInfo->sbdir;
        Ipp32s offsetY;
        Ipp32s pitch;
        Ipp32u block, i, loopCnt;
        Ipp32s xint, yint, xh, yh;
        Ipp32s mvx, mvy;
        Ipp32s width;
        Ipp32s height;
        Ipp32s xpos, ypos;
        IppiSize        roi = {-1, -1};

        // pointers for current subblock
        H264DecoderMotionVector *pMV_sb;
        Ipp8u *pRefY_sb;
        Ipp8u *pDstY_sb = NULL;
        Ipp8u *pTmpY = NULL;
        Ipp32s nTmpPitch = 16;

        Ipp32u uBlockDir;        // one of D_DIR_FWD, D_DIR_BWD, D_DIR_BIDIR
        bool bBidirWeightMB = false;    // is bidir weighting in effect for the MB?
        bool bUnidirWeightMB = false;    // is explicit L0 weighting in effect for the MB?
        bool bUnidirWeightSB = false;    // is explicit L0 weighting in effect for the subblock?

        // Optional weighting vars
        Ipp32u CurrPicParamSetId;
        Ipp32u weighted_bipred_idc = 0;
        Ipp32u luma_log2_weight_denom = 0;
        const Ipp32s *pDistScaleFactors = NULL;

        H264DecoderMotionVector *pMVFwd = NULL;
        H264DecoderMotionVector *pMVBwd = NULL;
        Ipp8s *pRefIndexL0 = NULL;
        Ipp8s *pRefIndexL1 = NULL;
        Ipp8s RefIndexL0 = 0;
        Ipp8s RefIndexL1 = 0;
        Ipp8s RefIndex = 0;

        Ipp32s temp_offset;

        VM_ASSERT(IS_INTER_MBTYPE(m_cur_mb.GlobalMacroblockInfo->mbtype));

        bool is_need_check_expand = !m_CurMB_Y || m_CurMB_Y > mb_height - 2;

        width = m_pCurrentFrame->lumaSize().width;
        height = m_pCurrentFrame->lumaSize().height;
        pitch = m_pCurrentFrame->pitch();
        //    subBlockPitch = m_pCurrentFrame->subBlockSize().width;


        offsetY = mbXOffset + mbYOffset*pitch;
        CurrPicParamSetId = m_pSliceHeader->pic_parameter_set_id;

        pMVFwd = m_cur_mb.MVs[0]->MotionVectors;
        pRefIndexL0 = m_cur_mb.RefIdxs[0]->RefIdxs;

        if (((PREDSLICE == m_pSliceHeader->slice_type) ||
            (S_PREDSLICE == m_pSliceHeader->slice_type)) &&
            (m_pPicParamSet[CurrPicParamSetId].weighted_pred_flag != 0))
        {
            // L0 weighting specified in pic param set. Get weighting params
            // for the slice.
            luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
            bUnidirWeightMB = true;
        }

        // get luma interp func pointer table in cache

        if (m_pSliceHeader->slice_type == BPREDSLICE)
        {
            VM_ASSERT(pRefPicList1[0]);
            pMVBwd = m_cur_mb.MVs[1]->MotionVectors;
            pRefIndexL1 = m_cur_mb.RefIdxs[1]->RefIdxs;
            // DIRECT MB have the same subblock partition structure as the
            // colocated MB. Take advantage of that to perform motion comp
            // for the direct MB using the largest partitions possible.
            if (mbtype == MBTYPE_DIRECT || mbtype == MBTYPE_SKIPPED)
            {
                mbtype = MBTYPE_INTER_8x8;
            }
            // Bi-dir weighting?
            weighted_bipred_idc = m_pPicParamSet[CurrPicParamSetId].weighted_bipred_idc;
            if (weighted_bipred_idc == 1)
            {
                // explicit bidir weighting
                luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
                bUnidirWeightMB = true;
                bBidirWeightMB = true;
            }
            if (weighted_bipred_idc == 2)
            {
                pDistScaleFactors = m_pSlice->GetDistScaleFactor();
                bBidirWeightMB = true;
            }
        }

        if (mbtype != MBTYPE_INTER_8x8 && mbtype != MBTYPE_INTER_8x8_REF0)
        {
            if (mbtype == MBTYPE_INTER_16x8)
            {
                roi.width        = 16;
                roi.height       = 8;
				this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_16x8;
            }
            else if (mbtype == MBTYPE_INTER_8x16)
            {
                roi.width        = 8;
                roi.height       = 16;
				this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x16;
            }
            else
            {
                roi.width          = 16;
                roi.height         = 16;
				this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_16x16;
            }
            VM_ASSERT(0 < (signed) roi.width);
            VM_ASSERT(0 < (signed) roi.height);
            block = 0;
            for (ypos=0; ypos<16; ypos+=roi.height)
            {
                for (xpos=0; xpos<16; xpos+=roi.width)
                {
                    if ((mbtype == MBTYPE_BIDIR) || (psbdir[block] == D_DIR_BIDIR))
                    {
                        uBlockDir = D_DIR_BIDIR;
                        loopCnt = 2;
                        bUnidirWeightSB = false;
                    }
                    else
                    {
                        loopCnt = 1;
                        if ((mbtype == MBTYPE_BACKWARD) ||
                            (psbdir[block] == D_DIR_BWD))
                            uBlockDir = D_DIR_BWD;
                        else
                            uBlockDir = D_DIR_FWD;
                        bUnidirWeightSB =  bUnidirWeightMB;
                    }

                    for (i=0; i<loopCnt; i++)
                    {
                        if ((uBlockDir == D_DIR_BWD) || i)
                        {
                            RefIndexL1 = RefIndex = pRefIndexL1 [(xpos>>2) + (ypos>>2)*4];
                            VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

							//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie: 
							//	return;
							//}

                            pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane + offsetY;

                            VM_ASSERT(pRefYPlane);

                            pMV = pMVBwd;
                        }
                        else
                        {
                            RefIndexL0 = RefIndex = pRefIndexL0 [(xpos>>2) + (ypos>>2)*4];

                            VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

							//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie: 
							//	return;
							//}

							pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane + offsetY;

                            VM_ASSERT(pRefYPlane);

                            pMV = pMVFwd;
                        }
                        // set pointers for this subblock
                        pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;
                        mvx = pMV_sb->mvx;
                        mvy = pMV_sb->mvy;

                        temp_offset = xpos + ypos*pitch;
                        pDstY_sb = pDstY + temp_offset;
                        pRefY_sb = pRefYPlane + temp_offset;

                        if (i > 0)
                        {
                         // advance Dst ptrs to next MB position, used as temp store
                         // for backward prediction. This is always OK because the Dst
                         // buffer is padded at the edges.
                            pTmpY = m_pPredictionBuffer;
                            nTmpPitch = 16;
                        }
                        else
                        {
                            pTmpY = pDstY_sb;
                            nTmpPitch = pitch;
                        }

                        xh = mvx & (INTERP_FACTOR-1);
                        yh = mvy & (INTERP_FACTOR-1);

                        // Note must select filter (get xh, yh) before clipping in
                        // order to preserve selection. Makes a difference only
                        // when 3,3 filter is selected. Now clip mvx and mvy
                        // rather than xint and yint to avoid clipping again for
                        // chroma.
                        Ipp8u pred_method = 0;
                        if (ABS(mvy) < (13 << INTERP_SHIFT))
                        {
                            if (is_need_check_expand)
                                pred_method = SelectPredictionMethod(
                                    mbYOffset+ypos,
                                    mvy,
                                    roi.height,
                                    height);
                        } else {
                            pred_method = SelectPredictionMethod(
                                mbYOffset+ypos,
                                mvy,
                                roi.height,
                                height);

                            mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                                1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                        }

                        if (ABS(mvx) < (D_MV_CLIP_LIMIT << INTERP_SHIFT))
                        {
                            mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width -
                                1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                        }

                        xint = mvx >> INTERP_SHIFT;
                        yint = mvy >> INTERP_SHIFT;

                        pRef = pRefY_sb + xint + yint * pitch;
                        switch(pred_method)
                        {
                        case ALLOK:
                            this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, roi);

                            break;
                        case PREDICTION_FROM_TOP:
                            ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                            break;
                        case PREDICTION_FROM_BOTTOM:
                            ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                            break;

                        default:VM_ASSERT(0);
                            break;
                        }

                        // optional prediction weighting
                        if (bUnidirWeightSB &&
                            m_pPredWeight[uBlockDir][RefIndex].luma_weight_flag != 0)
                        {
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpY, nTmpPitch,
                                luma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][RefIndex].luma_weight,
                                m_pPredWeight[uBlockDir][RefIndex].luma_offset,
                                roi);
                        }
                    }    // loopCnt
                    if (loopCnt > 1)
                    {
                        if (!bBidirWeightMB)
                        {
                            // combine bidir predictions into one, no weighting
                            // luma
                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb,
                                pTmpY,
                                pDstY_sb,
                                roi.width,
                                roi.height,
                                pitch,
                                nTmpPitch,
                                pitch);

                        }
                        else
                        {
                            // combine bidir predictions into one with weighting
                            if (weighted_bipred_idc == 1)
                            {
                                // combine bidir predictions into one, explicit weighting
                                // luma
                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    luma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].luma_weight,
                                    m_pPredWeight[0][RefIndexL0].luma_offset,
                                    m_pPredWeight[1][RefIndexL1].luma_weight,
                                    m_pPredWeight[1][RefIndexL1].luma_offset,
                                    roi);
                            }
                            else if (weighted_bipred_idc == 2)
                            {
                                // combine bidir predictions into one, implicit weighting
                                Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;
                                // luma
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi);
                            }
                            else
                                VM_ASSERT(0);
                        }    // weighted
                    }    // LoopCnt > 1
                    block++;
                }    // xpos
            }    // ypos
        }    // not 8x8
        else
        {
            IppiSize roi_8x8 = {8, 8};
            Ipp8u *psbtype = m_cur_mb.GlobalMacroblockInfo->sbtype;

            // MBTYPE_INTER_8x8:
            // 4 8x8 subblocks, each of which can be a different mode.
            for (block=0; block<4; block++)
            {
                switch (psbtype[block])
                {
                case SBTYPE_8x8:
                    roi.width        = 8;
                    roi.height       = 8;
					this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x8;
                    break;
                case SBTYPE_8x4:
                    roi.width        = 8;
                    roi.height       = 4;
					this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x4;
                    break;
                case SBTYPE_4x8:
                    roi.width        = 4;
                    roi.height       = 8;
					this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_4x8;
                    break;
                case SBTYPE_DIRECT:        // for spatial mode DIRECT
                    roi.width        = 4<<(Ipp8u)bUseDirect8x8Inference;
                    roi.height       = 4<<(Ipp8u)bUseDirect8x8Inference;
 					if( bUseDirect8x8Inference )
						this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_8x8;//***generic  bnie 2/3/2009
					else
						this->m_ippiInterpolateLuma_H264_8u_C1R = this->m_ippiInterpolateLuma_H264_8u_C1R_4x4;//***generic  bnie 2/3/2009
                   break;
                case SBTYPE_4x4:
                    roi.width        = 4;
                    roi.height       = 4;
                    break;
                default:
                    VM_ASSERT(0);
                    return;
                }    // switch sbtype

                if ((psbdir[block] == D_DIR_BIDIR) ||
                    (psbdir[block] == D_DIR_DIRECT) ||
                    (psbdir[block] == D_DIR_DIRECT_SPATIAL_BIDIR))
                {
                    uBlockDir = D_DIR_BIDIR;
                    loopCnt = 2;
                    bUnidirWeightSB =  false;
                }
                else
                {
                    loopCnt = 1;
                    if ((psbdir[block] == D_DIR_BWD) ||
                        (psbdir[block] == D_DIR_DIRECT_SPATIAL_BWD))
                        uBlockDir = D_DIR_BWD;
                    else
                        uBlockDir = D_DIR_FWD;
                    bUnidirWeightSB =  bUnidirWeightMB;
                }

                for (i=0; i<loopCnt; i++)
                {
                    xpos = xoff8[block];
                    ypos = yoff8[block];

                    if ((uBlockDir == D_DIR_BWD) || i)
                    {
                        RefIndexL1 = RefIndex = pRefIndexL1[(xpos>>2) + (ypos>>2)*4];

                        VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                            (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

						//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
						//{//***bnie: 
						//	return;
						//}
						
						pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane + offsetY;

                        VM_ASSERT(pRefYPlane);
                        VM_ASSERT(pRefVPlane);
                        VM_ASSERT(pRefUPlane);

                        pMV = pMVBwd;
                    }
                    else
                    {
                        RefIndexL0 = RefIndex = pRefIndexL0[(xpos>>2) + (ypos>>2)*4];

                        VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                            (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

						//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
						//{//***bnie: 
						//	return;
						//}

                        pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane + offsetY;

                        VM_ASSERT(pRefYPlane);
                        VM_ASSERT(pRefVPlane);
                        VM_ASSERT(pRefUPlane);

                        pMV = pMVFwd;
                    }

                    for (ypos=yoff8[block]; ypos<yoff8[block]+8; ypos+=roi.height)
                    {
                        for (xpos=xoff8[block]; xpos<xoff8[block]+8; xpos+=roi.width)
                        {
                            temp_offset = xpos + ypos*pitch;
                            pDstY_sb = pDstY + temp_offset;
                            pRefY_sb = pRefYPlane + temp_offset;

                            if (i > 0)
                            {
                                // advance Dst ptrs to next MB position, used as temp store
                                // for backward prediction. This is always OK because the Dst
                                // buffer is padded at the edges.
                                nTmpPitch = 16;
                                temp_offset = xpos + ypos*nTmpPitch;
                                pTmpY = m_pPredictionBuffer + temp_offset;
                            }
                            else
                            {
                                pTmpY = pDstY_sb;
                                nTmpPitch = pitch;
                            }

                            // set pointers for this subblock
                            pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;

                            mvx = pMV_sb->mvx;
                            mvy = pMV_sb->mvy;

                            xh = mvx & (INTERP_FACTOR-1);
                            yh = mvy & (INTERP_FACTOR-1);

                            Ipp8u pred_method=0;
                            if (ABS(mvy) < (13 << INTERP_SHIFT))
                            {
                                if (is_need_check_expand)
                                    pred_method=SelectPredictionMethod(
                                        mbYOffset+ypos,
                                        mvy,
                                        roi.height,
                                        height);
                            } else {
                                pred_method=SelectPredictionMethod(
                                    mbYOffset+ypos,
                                    mvy,
                                    roi.height,
                                    height);

                                mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                                    1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                                mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                            }

                            if (ABS(mvx) > (D_MV_CLIP_LIMIT << INTERP_SHIFT))
                            {
                                // See comment above about clipping & xh,yh.
                                mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width - 1
                                    - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                                mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                            }

                            xint = mvx >> INTERP_SHIFT;
                            yint = mvy >> INTERP_SHIFT;

                            pRef = pRefY_sb + xint + yint * pitch;
                            switch(pred_method)
                            {
                            case ALLOK:
                                this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, roi);

                                break;
                            case PREDICTION_FROM_TOP:
                                ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                                break;
                            case PREDICTION_FROM_BOTTOM:
                                ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                                break;

                            default:VM_ASSERT(0);
                                break;
                            }

                        }    // for xpos
                    }    // for ypos

                }    // loopCnt

                nTmpPitch = 16;
                temp_offset = xoff8[block] + yoff8[block]*nTmpPitch;
                pTmpY = m_pPredictionBuffer + temp_offset;

                temp_offset = xoff8[block] + yoff8[block]*pitch;
                pDstY_sb = pDstY + temp_offset;


                // optional prediction weighting
                if (bUnidirWeightSB &&
                    m_pPredWeight[uBlockDir][RefIndex].luma_weight_flag != 0)
                {
                    ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpY,
                        nTmpPitch,
                        luma_log2_weight_denom,
                        m_pPredWeight[uBlockDir][RefIndex].luma_weight,
                        m_pPredWeight[uBlockDir][RefIndex].luma_offset,
                        roi_8x8);

                }

                if (loopCnt > 1)
                {
                    if (!bBidirWeightMB)
                    {
                        // combine bidir predictions into one, no weighting
                        ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb,
                            pTmpY,
                            pDstY_sb,
                            roi_8x8.width,
                            roi_8x8.height,
                            pitch,
                            nTmpPitch,
                            pitch);

                    }
                    else
                    {
                        // combine bidir predictions into one with weighting
                        // combine bidir predictions into one with weighting
                        if (weighted_bipred_idc == 1)
                        {
                            // combine bidir predictions into one, explicit weighting

                            // luma
                            ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb,
                                pTmpY,
                                pDstY_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                luma_log2_weight_denom,
                                m_pPredWeight[0][RefIndexL0].luma_weight,
                                m_pPredWeight[0][RefIndexL0].luma_offset,
                                m_pPredWeight[1][RefIndexL1].luma_weight,
                                m_pPredWeight[1][RefIndexL1].luma_offset,
                                roi_8x8);
                        }
                        else if (weighted_bipred_idc == 2)
                        {
                            // combine bidir predictions into one, implicit weighting
                            Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;

                            // luma
                            ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb,
                                pTmpY,
                                pDstY_sb,
                                pitch,
                                nTmpPitch,
                                pitch,
                                64 - iDistScaleFactor,
                                iDistScaleFactor,
                                roi_8x8);
                        }
                        else
                            VM_ASSERT(0);
                    }    //  weighted
                }    // LoopCnt >1
            }    // for block
        }    // 8x8
    } // void H264SegmentDecoder::ReconstructMacroblock(Ipp8u *pDstY,

    void H264SegmentDecoder::ReconstructMacroblockH2(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1)
    {
        Ipp8u   *pRefYPlane;
        Ipp8u   *pRefVPlane;
        Ipp8u   *pRefUPlane;
        Ipp8u   *pRef;
        H264DecoderMotionVector *pMV;
        Ipp8s   *pRefIndex;
        Ipp32u mbtype = m_cur_mb.GlobalMacroblockInfo->mbtype;
        Ipp32u blocktype;

        Ipp8u *psbtype = m_cur_mb.GlobalMacroblockInfo->sbtype;
        Ipp8u *psbdir = m_cur_mb.LocalMacroblockInfo->sbdir;
        Ipp32s offsetY;
        Ipp32s offsetC;
        Ipp32s pitch;
        Ipp32u /*subBlockPitch,*/ block, i, loopCnt;
        Ipp32s xint, yint, xh, yh;
        Ipp32s mvx, mvy, mvyc;
        Ipp32s width;
        Ipp32s height;
        Ipp32s xpos, ypos;
        IppiSize        roi = {-1, -1}, roi_cr = {0};

        // pointers for current subblock
        H264DecoderMotionVector *pMV_sb;
        Ipp8u *pRefY_sb;
        Ipp8u *pDstY_sb = NULL;
        Ipp8u *pRefV_sb;
        Ipp8u *pDstV_sb = NULL;
        Ipp8u *pRefU_sb;
        Ipp8u *pDstU_sb = NULL;
        Ipp8u TemporaryBufferForMotionPrediction[16 * 16 * 3 + DEFAULT_ALIGN_VALUE];
        Ipp8u *pTmpY = NULL;
        Ipp8u *pTmpU = NULL;
        Ipp8u *pTmpV = NULL;
        Ipp32s nTmpPitch = 16;

        Ipp32u uBlockDir;        // one of D_DIR_FWD, D_DIR_BWD, D_DIR_BIDIR
        bool bBidirWeightMB = false;    // is bidir weighting in effect for the MB?
        bool bUnidirWeightMB = false;    // is explicit L0 weighting in effect for the MB?
        bool bUnidirWeightSB = false;    // is explicit L0 weighting in effect for the subblock?

        // Optional weighting vars
        Ipp32u CurrPicParamSetId;
        Ipp32u weighted_bipred_idc = 0;
        Ipp32u luma_log2_weight_denom = 0;
        Ipp32u chroma_log2_weight_denom = 0;
        const Ipp32s *pDistScaleFactors = NULL;

        H264DecoderMotionVector *pMVFwd = NULL;
        H264DecoderMotionVector *pMVBwd = NULL;
        Ipp8s *pRefIndexL0 = NULL;
        Ipp8s *pRefIndexL1 = NULL;
        Ipp8s RefIndexL0 = 0;
        Ipp8s RefIndexL1 = 0;


        VM_ASSERT(IS_INTER_MBTYPE(m_cur_mb.GlobalMacroblockInfo->mbtype));

        width = m_pCurrentFrame->lumaSize().width;
        height = m_pCurrentFrame->lumaSize().height;
        pitch = m_pCurrentFrame->pitch();
        //    subBlockPitch = m_pCurrentFrame->subBlockSize().width;


        offsetY = mbXOffset + mbYOffset*pitch;
        offsetC = (mbXOffset >> 1) +  ((mbYOffset) * pitch);
        CurrPicParamSetId = m_pSliceHeader->pic_parameter_set_id;

        pMVFwd = m_cur_mb.MVs[0]->MotionVectors;
        pRefIndexL0 = m_cur_mb.RefIdxs[0]->RefIdxs;

        if (((PREDSLICE == m_pSliceHeader->slice_type) ||
            (S_PREDSLICE == m_pSliceHeader->slice_type)) &&
            (m_pPicParamSet[CurrPicParamSetId].weighted_pred_flag != 0))
        {
            // L0 weighting specified in pic param set. Get weighting params
            // for the slice.
            luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
            chroma_log2_weight_denom = m_pSliceHeader->chroma_log2_weight_denom;
            bUnidirWeightMB = true;
        }

        // get luma interp func pointer table in cache

        if (m_pSliceHeader->slice_type == BPREDSLICE)
        {
            VM_ASSERT(pRefPicList1[0]);
            pMVBwd = m_cur_mb.MVs[1]->MotionVectors;
            pRefIndexL1 = m_cur_mb.RefIdxs[1]->RefIdxs;
            // DIRECT MB have the same subblock partition structure as the
            // colocated MB. Take advantage of that to perform motion comp
            // for the direct MB using the largest partitions possible.
            if (mbtype == MBTYPE_DIRECT || mbtype == MBTYPE_SKIPPED)
            {
                mbtype = MBTYPE_INTER_8x8;
            }
            // Bi-dir weighting?
            weighted_bipred_idc = m_pPicParamSet[CurrPicParamSetId].weighted_bipred_idc;
            if (weighted_bipred_idc == 1)
            {
                // explicit bidir weighting
                luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
                chroma_log2_weight_denom = m_pSliceHeader->chroma_log2_weight_denom;
                bUnidirWeightMB = true;
                bBidirWeightMB = true;
            }
            if (weighted_bipred_idc == 2)
            {
                pDistScaleFactors = m_pSlice->GetDistScaleFactor();
                bBidirWeightMB = true;
            }
        }

        if (mbtype != MBTYPE_INTER_8x8 && mbtype != MBTYPE_INTER_8x8_REF0)
        {
            if (mbtype == MBTYPE_INTER_16x8)
            {
                roi.width        = 16;
                roi.height       = 8;
                roi_cr.width     = 16>>1;
                roi_cr.height    = 8;
            }
            else if (mbtype == MBTYPE_INTER_8x16)
            {
                roi.width        = 8;
                roi.height       = 16;
                roi_cr.width     = 8>>1;
                roi_cr.height    = 16;
            }
            else
            {
                roi.width          = 16;
                roi.height         = 16;
                roi_cr.width       = 16>>1;
                roi_cr.height      = 16;
            }
            VM_ASSERT(0 < (signed) roi.width);
            VM_ASSERT(0 < (signed) roi.height);
            block = 0;
            for (ypos=0; ypos<16; ypos+=roi.height)
            {
                for (xpos=0; xpos<16; xpos+=roi.width)
                {
                    if ((mbtype == MBTYPE_BIDIR) || (psbdir[block] == D_DIR_BIDIR))
                    {
                        uBlockDir = D_DIR_BIDIR;
                        loopCnt = 2;
                        bUnidirWeightSB = false;
                    }
                    else
                    {
                        loopCnt = 1;
                        if ((mbtype == MBTYPE_BACKWARD) ||
                            (psbdir[block] == D_DIR_BWD))
                            uBlockDir = D_DIR_BWD;
                        else
                            uBlockDir = D_DIR_FWD;
                        bUnidirWeightSB =  bUnidirWeightMB;
                    }

                    for (i=0; i<loopCnt; i++)
                    {
                        if ((uBlockDir == D_DIR_BWD) || (i > 0))
                        {

                            pRefIndex = pRefIndexL1 + (xpos>>2) + (ypos>>2)*4;
                            RefIndexL1 = *pRefIndex;
                            VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

							//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie: 
							//	return;
							//}

                            pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane;
                            pRefVPlane = pRefPicList1[RefIndexL1]->m_pVPlane;
                            pRefUPlane = pRefPicList1[RefIndexL1]->m_pUPlane;

                            VM_ASSERT(pRefYPlane);
                            VM_ASSERT(pRefVPlane);
                            VM_ASSERT(pRefUPlane);

                            pMV = pMVBwd;
                        }
                        else
                        {
                            pRefIndex = pRefIndexL0 + (xpos>>2) + (ypos>>2)*4;
                            RefIndexL0 = *pRefIndex;

                            VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

							//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie: 
							//	return;
							//}

                            pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane;
                            pRefVPlane = pRefPicList0[RefIndexL0]->m_pVPlane;
                            pRefUPlane = pRefPicList0[RefIndexL0]->m_pUPlane;

                            VM_ASSERT(pRefYPlane);
                            VM_ASSERT(pRefVPlane);
                            VM_ASSERT(pRefUPlane);

                            pMV = pMVFwd;
                        }
                        // set pointers for this subblock
                        pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;
                        mvx = pMV_sb->mvx;
                        mvy = pMV_sb->mvy;

                        //offsetToBlock = xpos + ypos*pitch;
                        pDstY_sb = pDstY + xpos + ypos*pitch;//offsetToBlock;
                        pRefY_sb = pRefYPlane + xpos + ypos*pitch;//offsetToBlock;
                        pDstV_sb = pDstV + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);
                        pRefV_sb = pRefVPlane + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);
                        pDstU_sb = pDstU + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);
                        pRefU_sb = pRefUPlane + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);

                        if (i > 0)
                        {/*
                         // advance Dst ptrs to next MB position, used as temp store
                         // for backward prediction. This is always OK because the Dst
                         // buffer is padded at the edges.
                         pDstY_sb += 16;
                         pDstV_sb += 8;
                         pDstU_sb += 8;*/

                            pTmpY = align_pointer<Ipp8u *> (TemporaryBufferForMotionPrediction, DEFAULT_ALIGN_VALUE);
                            pTmpU = pTmpY + 16 * 16;
                            pTmpV = pTmpU + 16 * 16;
                            nTmpPitch = 16;
                        }
                        else
                        {
                            pTmpY = pDstY_sb;
                            pTmpU = pDstU_sb;
                            pTmpV = pDstV_sb;
                            nTmpPitch = pitch;
                        }

                        xh = mvx & (INTERP_FACTOR-1);
                        yh = mvy & (INTERP_FACTOR-1);

                        // Note must select filter (get xh, yh) before clipping in
                        // order to preserve selection. Makes a difference only
                        // when 3,3 filter is selected. Now clip mvx and mvy
                        // rather than xint and yint to avoid clipping again for
                        // chroma.
                        Ipp8u pred_method=SelectPredictionMethod(
                            mbYOffset+ypos,
                            mvy,
                            roi.height,
                            height);
                        mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width -
                            1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                        mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                        mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                            1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                        mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));

                        mvyc = mvy;

                        xint = mvx >> INTERP_SHIFT;
                        yint = mvy >> INTERP_SHIFT;

                        switch(pred_method)
                        {
                        case ALLOK:
                            pRef = pRefY_sb + offsetY + xint + yint * pitch;

                            this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, roi);

                            break;
                        case PREDICTION_FROM_TOP:
                            pRef = pRefY_sb + offsetY + xint + yint * pitch;

                            ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                            break;
                        case PREDICTION_FROM_BOTTOM:
                            pRef = pRefY_sb + offsetY + xint + yint * pitch;

                            ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                            break;

                        default:VM_ASSERT(0);
                            break;
                        }

                        // optional prediction weighting
                        if (bUnidirWeightSB &&
                            m_pPredWeight[uBlockDir][*pRefIndex].luma_weight_flag != 0)
                        {
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                luma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                                roi);
                            /*ippiWeightedAverage_H264_8u_C1IR( pDstY_sb,pDstY_sb,pitch,
                            m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                            0,
                            luma_log2_weight_denom,
                            m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                            roi);*/
                        }

                        // chroma (1/8 pixel MV)
                        xh = (mvx &  (((INTERP_FACTOR*2))-1));
                        yh = (mvyc & (((INTERP_FACTOR*2)>>1)-1))<<1;
                        xint = mvx >>  (INTERP_SHIFT+1);
                        yint = mvyc >> (INTERP_SHIFT);

                        switch(pred_method)
                        {
                        case ALLOK:
                            pRef = pRefV_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   roi_cr);

                            pRef = pRefU_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   roi_cr);
                            break;

                        case PREDICTION_FROM_TOP:
                            pRef = pRefV_sb + offsetC + xint + yint * pitch;


                            ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>>1)+yint), roi_cr);


                            pRef = pRefU_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>>1)+yint), roi_cr);

                            break;
                        case PREDICTION_FROM_BOTTOM:
                            pRef = pRefV_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);

                            pRef = pRefU_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);
                            break;
                        default:VM_ASSERT(0);
                            break;
                        }
                        // optional prediction weighting
                        if (bUnidirWeightSB &&
                            m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight_flag != 0)
                        {
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpV/*pDstV_sb*/,
                                nTmpPitch/*pitch*/,
                                chroma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                                roi_cr);
                            //ippiWeightedAverage_H264_8u_C1IR( pDstV_sb,pDstV_sb,pitch,
                            //                                      m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                            //                                     0,
                            //                                    chroma_log2_weight_denom,
                            //                                   m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                            //                                  roi_cr);
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpU/*pDstU_sb*/,
                                nTmpPitch/*pitch*/,
                                chroma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[0],
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[0],
                                roi_cr);
                            //ippiWeightedAverage_H264_8u_C1IR( pDstU_sb,pDstU_sb,pitch,
                            //                                       m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[0],
                            //                                       0,
                            //                                       chroma_log2_weight_denom,
                            //                                      m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[0],
                            //                                     roi_cr);

                        }
                    }    // loopCnt
                    if (loopCnt > 1)
                    {
                        if (!bBidirWeightMB)
                        {
                            // combine bidir predictions into one, no weighting
                            // luma
                            //(*m_InterpolBlock)(pDstY_sb-16, pDstY_sb, pDstY_sb-16, roi.width,
                            //                roi.height, pitch, pitch);

                            // chroma
                            //(*m_InterpolBlock)(pDstV_sb-8, pDstV_sb, pDstV_sb-8, roi.width>>1,
                            //                roi.height>>1, pitch, pitch);
                            //(*m_InterpolBlock)(pDstU_sb-8, pDstU_sb, pDstU_sb-8, roi.width>>1,
                            //                roi.height>>1, pitch, pitch);

                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                pTmpY/*pDstY_sb*/,
                                pDstY_sb/*-16*/,
                                roi.width,
                                roi.height,
                                pitch,
                                nTmpPitch,
                                pitch);
                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                pTmpV/*pDstV_sb*/,
                                pDstV_sb/*-8*/,
                                roi_cr.width,
                                roi_cr.height,
                                pitch,
                                nTmpPitch,
                                pitch);
                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                pTmpU/*pDstU_sb*/,
                                pDstU_sb/*-8*/,
                                roi_cr.width,
                                roi_cr.height,
                                pitch,
                                nTmpPitch,
                                pitch);
                        }
                        else
                        {
                            // combine bidir predictions into one with weighting
                            if (weighted_bipred_idc == 1)
                            {
                                // combine bidir predictions into one, explicit weighting

                                // luma
                                /*
                                ippiWeightedAverage_H264_8u_C1IR( pDstY_sb-16,pDstY_sb,pitch,
                                m_pPredWeight[0][RefIndexL0].luma_weight,
                                m_pPredWeight[1][RefIndexL1].luma_weight,
                                luma_log2_weight_denom,
                                (m_pPredWeight[0][RefIndexL0].luma_offset+
                                m_pPredWeight[1][RefIndexL1].luma_offset+1)>>1,
                                roi);
                                ippiWeightedAverage_H264_8u_C1IR( pDstV_sb-8,pDstY_sb,pitch,
                                m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                chroma_log2_weight_denom,
                                (m_pPredWeight[0][RefIndexL0].chroma_offset[1]+
                                m_pPredWeight[1][RefIndexL1].chroma_offset[1]+1)>>1,
                                roi_cr);
                                ippiWeightedAverage_H264_8u_C1IR( pDstU_sb-8,pDstU_sb,pitch,
                                m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                chroma_log2_weight_denom,
                                (m_pPredWeight[0][RefIndexL0].chroma_offset[0]+
                                m_pPredWeight[1][RefIndexL1].chroma_offset[0]+1)>>1,
                                roi_cr);*/
                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    luma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].luma_weight,
                                    m_pPredWeight[0][RefIndexL0].luma_offset,
                                    m_pPredWeight[1][RefIndexL1].luma_weight,
                                    m_pPredWeight[1][RefIndexL1].luma_offset,
                                    roi);
                                // chroma
                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                    pTmpV/*pDstV_sb*/,
                                    pDstV_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                    m_pPredWeight[0][RefIndexL0].chroma_offset[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[1],
                                    roi_cr);

                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                    pTmpU/*pDstU_sb*/,
                                    pDstU_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                    m_pPredWeight[0][RefIndexL0].chroma_offset[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[0],
                                    roi_cr);
                            }
                            else if (weighted_bipred_idc == 2)
                            {
                                // combine bidir predictions into one, implicit weighting
                                Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;
                                // luma
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi);
                                // chroma
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                    pTmpV/*pDstV_sb*/,
                                    pDstV_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi_cr);
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                    pTmpU/*pDstU_sb*/,
                                    pDstU_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi_cr);
                            }
                            else
                                VM_ASSERT(0);
                        }    // weighted
                    }    // LoopCnt > 1
                    block++;
                }    // xpos
            }    // ypos
        }    // not 8x8
        else
        {
            // MBTYPE_INTER_8x8:
            // 4 8x8 subblocks, each of which can be a different mode.
            for (block=0; block<4; block++)
            {
                {
                    blocktype = psbtype[block];
                }
                switch (blocktype)
                {
                case SBTYPE_8x8:
                    roi.width        = 8;
                    roi.height       = 8;
                    roi_cr.width     = 8>>1;
                    roi_cr.height    = 8;
                    break;
                case SBTYPE_8x4:
                    roi.width        = 8;
                    roi.height       = 4;
                    roi_cr.width     = 8>>1;
                    roi_cr.height    = 4;
                    break;
                case SBTYPE_4x8:
                    roi.width        = 4;
                    roi.height       = 8;
                    roi_cr.width     = 4>>1;
                    roi_cr.height    = 8;
                    break;
                case SBTYPE_DIRECT:        // for spatial mode DIRECT
                    roi.width        = 4<<(Ipp8u)bUseDirect8x8Inference;
                    roi.height       = 4<<(Ipp8u)bUseDirect8x8Inference;
                    roi_cr.width     = (4<<(Ipp8u)bUseDirect8x8Inference)>>1;
                    roi_cr.height    = (4<<(Ipp8u)bUseDirect8x8Inference);
                    break;
                case SBTYPE_4x4:
                    roi.width        = 4;
                    roi.height       = 4;
                    roi_cr.width     = 4>>1;
                    roi_cr.height    = 4;
                    break;
                default:
                    VM_ASSERT(0);
                    return;
                }    // switch sbtype

                for (ypos=yoff8[block]; ypos<yoff8[block]+8; ypos+=roi.height)
                {
                    for (xpos=xoff8[block]; xpos<xoff8[block]+8; xpos+=roi.width)
                    {
                        if ((psbdir[block] == D_DIR_BIDIR) ||
                            (psbdir[block] == D_DIR_DIRECT) ||
                            (psbdir[block] == D_DIR_DIRECT_SPATIAL_BIDIR))
                        {
                            uBlockDir = D_DIR_BIDIR;
                            loopCnt = 2;
                            bUnidirWeightSB =  false;
                        }
                        else
                        {
                            loopCnt = 1;
                            if ((psbdir[block] == D_DIR_BWD) ||
                                (psbdir[block] == D_DIR_DIRECT_SPATIAL_BWD))
                                uBlockDir = D_DIR_BWD;
                            else
                                uBlockDir = D_DIR_FWD;
                            bUnidirWeightSB =  bUnidirWeightMB;
                        }

                        for (i=0; i<loopCnt; i++)
                        {
                            if ((uBlockDir == D_DIR_BWD) || (i > 0))
                            {
                                pRefIndex = pRefIndexL1 + (xpos>>2) + (ypos>>2)*4;
                                RefIndexL1 = *pRefIndex;

                                VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                                    (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

 								//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
								//{//***bnie: 
								//	return;
								//}

								pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane;
                                pRefVPlane = pRefPicList1[RefIndexL1]->m_pVPlane;
                                pRefUPlane = pRefPicList1[RefIndexL1]->m_pUPlane;

                                VM_ASSERT(pRefYPlane);
                                VM_ASSERT(pRefVPlane);
                                VM_ASSERT(pRefUPlane);

                                pMV = pMVBwd;
                            }
                            else
                            {
                                pRefIndex = pRefIndexL0 + (xpos>>2) + (ypos>>2)*4;
                                RefIndexL0 = *pRefIndex;


                                VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                                    (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

 								//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
								//{//***bnie: 
								//	return;
								//}

								pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane;
                                pRefVPlane = pRefPicList0[RefIndexL0]->m_pVPlane;
                                pRefUPlane = pRefPicList0[RefIndexL0]->m_pUPlane;

                                VM_ASSERT(pRefYPlane);
                                VM_ASSERT(pRefVPlane);
                                VM_ASSERT(pRefUPlane);

                                pMV = pMVFwd;
                            }
                            // set pointers for this subblock
                            pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;

                            mvx = pMV_sb->mvx;
                            mvy = pMV_sb->mvy;

                            //offsetToBlock = xpos + ypos*pitch;
                            pDstY_sb = pDstY + xpos + ypos*pitch;//offsetToBlock;
                            pRefY_sb = pRefYPlane + xpos + ypos*pitch;//offsetToBlock;
                            pDstV_sb = pDstV + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);
                            pRefV_sb = pRefVPlane + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);
                            pDstU_sb = pDstU + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);
                            pRefU_sb = pRefUPlane + (xpos>>1) + (ypos)*pitch;//(offsetToBlock>>1);

                            if (i > 0)
                            {/*
                             // advance Dst ptrs to next MB position, used as temp store
                             // for backward prediction. This is always OK because the Dst
                             // buffer is padded at the edges.
                             pDstY_sb += 16;
                             pDstV_sb += 8;
                             pDstU_sb += 8;*/

                                pTmpY = align_pointer<Ipp8u *> (TemporaryBufferForMotionPrediction, DEFAULT_ALIGN_VALUE);
                                pTmpU = pTmpY + 16 * 16;
                                pTmpV = pTmpU + 16 * 16;
                                nTmpPitch = 16;
                            }
                            else
                            {
                                pTmpY = pDstY_sb;
                                pTmpU = pDstU_sb;
                                pTmpV = pDstV_sb;
                                nTmpPitch = pitch;
                            }

                            xh = mvx & (INTERP_FACTOR-1);
                            yh = mvy & (INTERP_FACTOR-1);

                            Ipp8u pred_method=SelectPredictionMethod(
                                mbYOffset+ypos,
                                mvy,
                                roi.height,
                                height);
                            // See comment above about clipping & xh,yh.
                            mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width - 1
                                - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                            mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                                1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));

                            mvyc = mvy;

                            xint = mvx >> INTERP_SHIFT;
                            yint = mvy >> INTERP_SHIFT;

                            switch(pred_method)
                            {
                            case ALLOK:
                                pRef = pRefY_sb + offsetY + xint + yint * pitch;

                                this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, roi);

                                break;
                            case PREDICTION_FROM_TOP:
                                pRef = pRefY_sb + offsetY + xint + yint * pitch;

                                ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                                break;
                            case PREDICTION_FROM_BOTTOM:
                                pRef = pRefY_sb + offsetY + xint + yint * pitch;

                                ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                                break;

                            default:VM_ASSERT(0);
                                break;
                            }

                            // optional prediction weighting
                            if (bUnidirWeightSB &&
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_weight_flag != 0)
                            {
                                ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpY/*pDstY_sb*/,
                                    nTmpPitch/*pitch*/,
                                    luma_log2_weight_denom,
                                    m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                                    m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                                    roi);
                                /*
                                ippiWeightedAverage_H264_8u_C1IR( pDstY_sb,pDstY_sb,pitch,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                                0,
                                luma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                                roi);*/

                            }
                            // chroma (1/8 pixel MV)
                            xh = (mvx &  (((INTERP_FACTOR*2)>>1)-1));
                            yh = (mvyc & (((INTERP_FACTOR*2))-1))<<1;
                            xint = mvx >>  (INTERP_SHIFT+1);
                            yint = mvyc >> (INTERP_SHIFT);

                            switch(pred_method)
                            {
                            case ALLOK:
                                pRef = pRefV_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   roi_cr);

                                pRef = pRefU_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   roi_cr);
                                break;

                            case PREDICTION_FROM_TOP:
                                pRef = pRefV_sb + offsetC + xint + yint * pitch;


                                ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>>1)+yint), roi_cr);


                                pRef = pRefU_sb + (offsetY>>1) + xint + yint * pitch;

                                ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,  - ((((Ipp32s)mbYOffset+ypos)>>1)+yint), roi_cr);

                                break;
                            case PREDICTION_FROM_BOTTOM:
                                pRef = pRefV_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);

                                pRef = pRefU_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);
                                break;
                            default:VM_ASSERT(0);
                                break;
                            }
                            // optional prediction weighting
                            if (bUnidirWeightSB &&
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight_flag != 0)
                            {
                                ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpV/*pDstV_sb*/,
                                    nTmpPitch/*pitch*/,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                                    roi_cr);
                                ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpU/*pDstU_sb*/,
                                    nTmpPitch/*pitch*/,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[0],
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[0],
                                    roi_cr);
                                //ippiWeightedAverage_H264_8u_C1IR( pDstV_sb,pDstV_sb,pitch,
                                //                                       m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                                //                                      0,
                                //                                      chroma_log2_weight_denom,
                                //                                      m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                                //                                      roi_cr);

                                //ippiWeightedAverage_H264_8u_C1IR( pDstU_sb,pDstU_sb,pitch,
                                //                                       m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[0],
                                //                                       0,
                                //                                       chroma_log2_weight_denom,
                                //                                       m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[0],
                                //                                       roi_cr);

                            }
                        }    // loopCnt
                        if (loopCnt > 1)
                        {
                            if (!bBidirWeightMB)
                            {
                                // combine bidir predictions into one, no weighting

                                // luma
                                //                            (*m_InterpolBlock)(pDstY_sb-16, pDstY_sb, pDstY_sb-16, roi.width,
                                //                                            roi.height, pitch, pitch);
                                // chroma
                                //                            (*m_InterpolBlock)(pDstV_sb-8, pDstV_sb, pDstV_sb-8, roi.width>>1,
                                //                                            roi.height>>1, pitch, pitch);
                                //                            (*m_InterpolBlock)(pDstU_sb-8, pDstU_sb, pDstU_sb-8, roi.width>>1,
                                //                                            roi.height>>1, pitch, pitch);
                                ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    roi.width,
                                    roi.height,
                                    pitch,
                                    nTmpPitch,
                                    pitch);
                                ippiInterpolateBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                    pTmpV/*pDstV_sb*/,
                                    pDstV_sb/*-8*/,
                                    roi_cr.width,
                                    roi_cr.height,
                                    pitch,
                                    nTmpPitch,
                                    pitch);
                                ippiInterpolateBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                    pTmpU/*pDstU_sb*/,
                                    pDstU_sb/*-8*/,
                                    roi_cr.width,
                                    roi_cr.height,
                                    pitch,
                                    nTmpPitch,
                                    pitch);
                            }
                            else
                            {
                                // combine bidir predictions into one with weighting
                                // combine bidir predictions into one with weighting
                                if (weighted_bipred_idc == 1)
                                {
                                    // combine bidir predictions into one, explicit weighting

                                    // luma
                                    /*
                                    ippiWeightedAverage_H264_8u_C1IR( pDstY_sb-16,pDstY_sb,pitch,
                                    m_pPredWeight[0][RefIndexL0].luma_weight,
                                    m_pPredWeight[1][RefIndexL1].luma_weight,
                                    luma_log2_weight_denom,
                                    (m_pPredWeight[0][RefIndexL0].luma_offset+
                                    m_pPredWeight[1][RefIndexL1].luma_offset+1)>>1,
                                    roi);
                                    ippiWeightedAverage_H264_8u_C1IR( pDstV_sb-8,pDstY_sb,pitch,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                    chroma_log2_weight_denom,
                                    (m_pPredWeight[0][RefIndexL0].chroma_offset[1]+
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[1]+1)>>1,
                                    roi_cr);
                                    ippiWeightedAverage_H264_8u_C1IR( pDstU_sb-8,pDstU_sb,pitch,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                    chroma_log2_weight_denom,
                                    (m_pPredWeight[0][RefIndexL0].chroma_offset[0]+
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[0]+1)>>1,
                                    roi_cr);*/
                                    ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                        pTmpY/*pDstY_sb*/,
                                        pDstY_sb/*-16*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        luma_log2_weight_denom,
                                        m_pPredWeight[0][RefIndexL0].luma_weight,
                                        m_pPredWeight[0][RefIndexL0].luma_offset,
                                        m_pPredWeight[1][RefIndexL1].luma_weight,
                                        m_pPredWeight[1][RefIndexL1].luma_offset,
                                        roi);
                                    // chroma
                                    ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                        pTmpV/*pDstV_sb*/,
                                        pDstV_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        chroma_log2_weight_denom,
                                        m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                        m_pPredWeight[0][RefIndexL0].chroma_offset[1],
                                        m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                        m_pPredWeight[1][RefIndexL1].chroma_offset[1],
                                        roi_cr);
                                    ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                        pTmpU/*pDstU_sb*/,
                                        pDstU_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        chroma_log2_weight_denom,
                                        m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                        m_pPredWeight[0][RefIndexL0].chroma_offset[0],
                                        m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                        m_pPredWeight[1][RefIndexL1].chroma_offset[0],
                                        roi_cr);
                                }
                                else if (weighted_bipred_idc == 2)
                                {
                                    // combine bidir predictions into one, implicit weighting
                                    Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;

                                    // luma
                                    ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                        pTmpY/*pDstY_sb*/,
                                        pDstY_sb/*-16*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        64 - iDistScaleFactor,
                                        iDistScaleFactor,
                                        roi);
                                    // chroma
                                    ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                        pTmpV/*pDstV_sb*/,
                                        pDstV_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        64 - iDistScaleFactor,
                                        iDistScaleFactor,
                                        roi_cr);
                                    ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                        pTmpU/*pDstU_sb*/,
                                        pDstU_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        64 - iDistScaleFactor,
                                        iDistScaleFactor,
                                        roi_cr);
                                }
                                else
                                    VM_ASSERT(0);
                            }    //  weighted
                        }    // LoopCnt >1
                    }    // for xpos
                }    // for ypos
            }    // for block
        }    // 8x8

    } // void H264SegmentDecoder::ReconstructMacroblock(Ipp8u *pDstY,

    void H264SegmentDecoder::ReconstructMacroblockH4(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1)
    {
        Ipp8u   *pRefYPlane;
        Ipp8u   *pRefVPlane;
        Ipp8u   *pRefUPlane;
        Ipp8u   *pRef;
        H264DecoderMotionVector *pMV;
        Ipp8s   *pRefIndex;
        Ipp32u mbtype = m_cur_mb.GlobalMacroblockInfo->mbtype;
        Ipp32u blocktype;

        Ipp8u *psbtype = m_cur_mb.GlobalMacroblockInfo->sbtype;
        Ipp8u *psbdir = m_cur_mb.LocalMacroblockInfo->sbdir;
        Ipp32s offsetY;
        Ipp32s offsetC;
        Ipp32s pitch;
        Ipp32u /*subBlockPitch,*/ block, i, loopCnt;
        Ipp32s xint, yint, xh, yh;
        Ipp32s mvx, mvy, mvyc;
        Ipp32s width;
        Ipp32s height;
        Ipp32s xpos, ypos;
        IppiSize        roi = {-1, -1}, roi_cr = {0};

        // pointers for current subblock
        H264DecoderMotionVector *pMV_sb;
        Ipp8u *pRefY_sb;
        Ipp8u *pDstY_sb = NULL;
        Ipp8u *pRefV_sb;
        Ipp8u *pDstV_sb = NULL;
        Ipp8u *pRefU_sb;
        Ipp8u *pDstU_sb = NULL;
        Ipp8u TemporaryBufferForMotionPrediction[16 * 16 * 3 + DEFAULT_ALIGN_VALUE];
        Ipp8u *pTmpY = NULL;
        Ipp8u *pTmpU = NULL;
        Ipp8u *pTmpV = NULL;
        Ipp32s nTmpPitch = 16;

        Ipp32u uBlockDir;        // one of D_DIR_FWD, D_DIR_BWD, D_DIR_BIDIR
        bool bBidirWeightMB = false;    // is bidir weighting in effect for the MB?
        bool bUnidirWeightMB = false;    // is explicit L0 weighting in effect for the MB?
        bool bUnidirWeightSB = false;    // is explicit L0 weighting in effect for the subblock?

        // Optional weighting vars
        Ipp32u CurrPicParamSetId;
        Ipp32u weighted_bipred_idc = 0;
        Ipp32u luma_log2_weight_denom = 0;
        Ipp32u chroma_log2_weight_denom = 0;
        const Ipp32s *pDistScaleFactors = NULL;

        H264DecoderMotionVector *pMVFwd = NULL;
        H264DecoderMotionVector *pMVBwd = NULL;
        Ipp8s *pRefIndexL0 = NULL;
        Ipp8s *pRefIndexL1 = NULL;
        Ipp8s RefIndexL0 = 0;
        Ipp8s RefIndexL1 = 0;


        VM_ASSERT(IS_INTER_MBTYPE(m_cur_mb.GlobalMacroblockInfo->mbtype));

        width = m_pCurrentFrame->lumaSize().width;
        height = m_pCurrentFrame->lumaSize().height;
        pitch = m_pCurrentFrame->pitch();
        //    subBlockPitch = m_pCurrentFrame->subBlockSize().width;


        offsetY = mbXOffset + mbYOffset*pitch;
        offsetC = (mbXOffset) +  ((mbYOffset) * pitch);
        CurrPicParamSetId = m_pSliceHeader->pic_parameter_set_id;

        pMVFwd = m_cur_mb.MVs[0]->MotionVectors;
        pRefIndexL0 = m_cur_mb.RefIdxs[0]->RefIdxs;

        if (((PREDSLICE == m_pSliceHeader->slice_type) ||
            (S_PREDSLICE == m_pSliceHeader->slice_type)) &&
            (m_pPicParamSet[CurrPicParamSetId].weighted_pred_flag != 0))
        {
            // L0 weighting specified in pic param set. Get weighting params
            // for the slice.
            luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
            chroma_log2_weight_denom = m_pSliceHeader->chroma_log2_weight_denom;
            bUnidirWeightMB = true;
        }

        // get luma interp func pointer table in cache

        if (m_pSliceHeader->slice_type == BPREDSLICE)
        {
            VM_ASSERT(pRefPicList1[0]);
            pMVBwd = m_cur_mb.MVs[1]->MotionVectors;
            pRefIndexL1 = m_cur_mb.RefIdxs[1]->RefIdxs;
            // DIRECT MB have the same subblock partition structure as the
            // colocated MB. Take advantage of that to perform motion comp
            // for the direct MB using the largest partitions possible.
            if (mbtype == MBTYPE_DIRECT || mbtype == MBTYPE_SKIPPED)
            {
                mbtype = MBTYPE_INTER_8x8;
            }
            // Bi-dir weighting?
            weighted_bipred_idc = m_pPicParamSet[CurrPicParamSetId].weighted_bipred_idc;
            if (weighted_bipred_idc == 1)
            {
                // explicit bidir weighting
                luma_log2_weight_denom = m_pSliceHeader->luma_log2_weight_denom;
                chroma_log2_weight_denom = m_pSliceHeader->chroma_log2_weight_denom;
                bUnidirWeightMB = true;
                bBidirWeightMB = true;
            }
            if (weighted_bipred_idc == 2)
            {
                pDistScaleFactors = m_pSlice->GetDistScaleFactor();
                bBidirWeightMB = true;
            }
        }

        if (mbtype != MBTYPE_INTER_8x8 && mbtype != MBTYPE_INTER_8x8_REF0)
        {
            if (mbtype == MBTYPE_INTER_16x8)
            {
                roi.width        = 16;
                roi.height       = 8;
                roi_cr.width     = 16;
                roi_cr.height    = 8;
            }
            else if (mbtype == MBTYPE_INTER_8x16)
            {
                roi.width        = 8;
                roi.height       = 16;
                roi_cr.width     = 8;
                roi_cr.height    = 16;
            }
            else
            {
                roi.width          = 16;
                roi.height         = 16;
                roi_cr.width       = 16;
                roi_cr.height      = 16;
            }
            VM_ASSERT(0 < (signed) roi.width);
            VM_ASSERT(0 < (signed) roi.height);
            block = 0;
            for (ypos=0; ypos<16; ypos+=roi.height)
            {
                for (xpos=0; xpos<16; xpos+=roi.width)
                {
                    if ((mbtype == MBTYPE_BIDIR) || (psbdir[block] == D_DIR_BIDIR))
                    {
                        uBlockDir = D_DIR_BIDIR;
                        loopCnt = 2;
                        bUnidirWeightSB = false;
                    }
                    else
                    {
                        loopCnt = 1;
                        if ((mbtype == MBTYPE_BACKWARD) ||
                            (psbdir[block] == D_DIR_BWD))
                            uBlockDir = D_DIR_BWD;
                        else
                            uBlockDir = D_DIR_FWD;
                        bUnidirWeightSB =  bUnidirWeightMB;
                    }

                    for (i=0; i<loopCnt; i++)
                    {
                        if ((uBlockDir == D_DIR_BWD) || (i > 0))
                        {

                            pRefIndex = pRefIndexL1 + (xpos>>2) + (ypos>>2)*4;
                            RefIndexL1 = *pRefIndex;
                            VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

							//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie: 
							//	return;
							//}

                            pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane;
                            pRefVPlane = pRefPicList1[RefIndexL1]->m_pVPlane;
                            pRefUPlane = pRefPicList1[RefIndexL1]->m_pUPlane;

                            VM_ASSERT(pRefYPlane);
                            VM_ASSERT(pRefVPlane);
                            VM_ASSERT(pRefUPlane);

                            pMV = pMVBwd;
                        }
                        else
                        {
                            pRefIndex = pRefIndexL0 + (xpos>>2) + (ypos>>2)*4;
                            RefIndexL0 = *pRefIndex;

                            VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                                (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

							//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
							//{//***bnie: 
							//	return;
							//}

							pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane;
                            pRefVPlane = pRefPicList0[RefIndexL0]->m_pVPlane;
                            pRefUPlane = pRefPicList0[RefIndexL0]->m_pUPlane;

                            VM_ASSERT(pRefYPlane);
                            VM_ASSERT(pRefVPlane);
                            VM_ASSERT(pRefUPlane);

                            pMV = pMVFwd;
                        }
                        // set pointers for this subblock
                        pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;
                        mvx = pMV_sb->mvx;
                        mvy = pMV_sb->mvy;

                        //offsetToBlock = xpos + ypos*pitch;
                        pDstY_sb = pDstY + xpos + ypos*pitch;//offsetToBlock;
                        pRefY_sb = pRefYPlane + xpos + ypos*pitch;//offsetToBlock;
                        pDstV_sb = pDstV + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);
                        pRefV_sb = pRefVPlane + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);
                        pDstU_sb = pDstU + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);
                        pRefU_sb = pRefUPlane + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);

                        if (i > 0)
                        {/*
                         // advance Dst ptrs to next MB position, used as temp store
                         // for backward prediction. This is always OK because the Dst
                         // buffer is padded at the edges.
                         pDstY_sb += 16;
                         pDstV_sb += 8;
                         pDstU_sb += 8;*/

                            pTmpY = align_pointer<Ipp8u *> (TemporaryBufferForMotionPrediction, DEFAULT_ALIGN_VALUE);
                            pTmpU = pTmpY + 16 * 16;
                            pTmpV = pTmpU + 16 * 16;
                            nTmpPitch = 16;
                        }
                        else
                        {
                            pTmpY = pDstY_sb;
                            pTmpU = pDstU_sb;
                            pTmpV = pDstV_sb;
                            nTmpPitch = pitch;
                        }

                        xh = mvx & (INTERP_FACTOR-1);
                        yh = mvy & (INTERP_FACTOR-1);

                        // Note must select filter (get xh, yh) before clipping in
                        // order to preserve selection. Makes a difference only
                        // when 3,3 filter is selected. Now clip mvx and mvy
                        // rather than xint and yint to avoid clipping again for
                        // chroma.
                        Ipp8u pred_method=SelectPredictionMethod(
                            mbYOffset+ypos,
                            mvy,
                            roi.height,
                            height);
                        mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width -
                            1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                        mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                        mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                            1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                        mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));

                        mvyc = mvy;

                        xint = mvx >> INTERP_SHIFT;
                        yint = mvy >> INTERP_SHIFT;

                        switch(pred_method)
                        {
                        case ALLOK:
                            pRef = pRefY_sb + offsetY + xint + yint * pitch;

                            this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, roi);

                            break;
                        case PREDICTION_FROM_TOP:
                            pRef = pRefY_sb + offsetY + xint + yint * pitch;

                            ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                            break;
                        case PREDICTION_FROM_BOTTOM:
                            pRef = pRefY_sb + offsetY + xint + yint * pitch;

                            ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                            break;

                        default:VM_ASSERT(0);
                            break;
                        }

                        // optional prediction weighting
                        if (bUnidirWeightSB &&
                            m_pPredWeight[uBlockDir][*pRefIndex].luma_weight_flag != 0)
                        {
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                luma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                                roi);
                            /*ippiWeightedAverage_H264_8u_C1IR( pDstY_sb,pDstY_sb,pitch,
                            m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                            0,
                            luma_log2_weight_denom,
                            m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                            roi);*/
                        }
                        // chroma (1/8 pixel MV)
                        xh = (mvx &  (((INTERP_FACTOR*2)>>1)-1))<<1;
                        yh = (mvyc & (((INTERP_FACTOR*2)>>1)-1))<<1;
                        xint = mvx >>  (INTERP_SHIFT);
                        yint = mvyc >> (INTERP_SHIFT);

                        switch(pred_method)
                        {
                        case ALLOK:
                            pRef = pRefV_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   roi_cr);

                            pRef = pRefU_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   roi_cr);
                            break;

                        case PREDICTION_FROM_TOP:
                            pRef = pRefV_sb + offsetC + xint + yint * pitch;


                            ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,  - ((((Ipp32s)mbYOffset+ypos))+yint), roi_cr);


                            pRef = pRefU_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,  - ((((Ipp32s)mbYOffset+ypos))+yint), roi_cr);

                            break;
                        case PREDICTION_FROM_BOTTOM:
                            pRef = pRefV_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);

                            pRef = pRefU_sb + offsetC + xint + yint * pitch;

                            ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);
                            break;
                        default:VM_ASSERT(0);
                            break;
                        }
                        // optional prediction weighting
                        if (bUnidirWeightSB &&
                            m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight_flag != 0)
                        {
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpV/*pDstV_sb*/,
                                nTmpPitch/*pitch*/,
                                chroma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                                roi_cr);
                            //ippiWeightedAverage_H264_8u_C1IR( pDstV_sb,pDstV_sb,pitch,
                            //                                      m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                            //                                     0,
                            //                                    chroma_log2_weight_denom,
                            //                                   m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                            //                                  roi_cr);
                            ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpU/*pDstU_sb*/,
                                nTmpPitch/*pitch*/,
                                chroma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[0],
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[0],
                                roi_cr);

                        }
                    }    // loopCnt
                    if (loopCnt > 1)
                    {
                        if (!bBidirWeightMB)
                        {
                            // combine bidir predictions into one, no weighting
                            // luma
                            //(*m_InterpolBlock)(pDstY_sb-16, pDstY_sb, pDstY_sb-16, roi.width,
                            //                roi.height, pitch, pitch);

                            // chroma
                            //(*m_InterpolBlock)(pDstV_sb-8, pDstV_sb, pDstV_sb-8, roi.width>>1,
                            //                roi.height>>1, pitch, pitch);
                            //(*m_InterpolBlock)(pDstU_sb-8, pDstU_sb, pDstU_sb-8, roi.width>>1,
                            //                roi.height>>1, pitch, pitch);

                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                pTmpY/*pDstY_sb*/,
                                pDstY_sb/*-16*/,
                                roi.width,
                                roi.height,
                                pitch,
                                nTmpPitch,
                                pitch);

                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                pTmpV/*pDstV_sb*/,
                                pDstV_sb/*-8*/,
                                roi_cr.width,
                                roi_cr.height,
                                pitch,
                                nTmpPitch,
                                pitch);
                            ippiInterpolateBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                pTmpU/*pDstU_sb*/,
                                pDstU_sb/*-8*/,
                                roi_cr.width,
                                roi_cr.height,
                                pitch,
                                nTmpPitch,
                                pitch);

                        }
                        else
                        {
                            // combine bidir predictions into one with weighting
                            if (weighted_bipred_idc == 1)
                            {
                                // combine bidir predictions into one, explicit weighting

                                // luma
                                /*
                                ippiWeightedAverage_H264_8u_C1IR( pDstY_sb-16,pDstY_sb,pitch,
                                m_pPredWeight[0][RefIndexL0].luma_weight,
                                m_pPredWeight[1][RefIndexL1].luma_weight,
                                luma_log2_weight_denom,
                                (m_pPredWeight[0][RefIndexL0].luma_offset+
                                m_pPredWeight[1][RefIndexL1].luma_offset+1)>>1,
                                roi);
                                ippiWeightedAverage_H264_8u_C1IR( pDstV_sb-8,pDstY_sb,pitch,
                                m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                chroma_log2_weight_denom,
                                (m_pPredWeight[0][RefIndexL0].chroma_offset[1]+
                                m_pPredWeight[1][RefIndexL1].chroma_offset[1]+1)>>1,
                                roi_cr);
                                ippiWeightedAverage_H264_8u_C1IR( pDstU_sb-8,pDstU_sb,pitch,
                                m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                chroma_log2_weight_denom,
                                (m_pPredWeight[0][RefIndexL0].chroma_offset[0]+
                                m_pPredWeight[1][RefIndexL1].chroma_offset[0]+1)>>1,
                                roi_cr);*/
                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    luma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].luma_weight,
                                    m_pPredWeight[0][RefIndexL0].luma_offset,
                                    m_pPredWeight[1][RefIndexL1].luma_weight,
                                    m_pPredWeight[1][RefIndexL1].luma_offset,
                                    roi);
                                // chroma
                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                    pTmpV/*pDstV_sb*/,
                                    pDstV_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                    m_pPredWeight[0][RefIndexL0].chroma_offset[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[1],
                                    roi_cr);

                                ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                    pTmpU/*pDstU_sb*/,
                                    pDstU_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                    m_pPredWeight[0][RefIndexL0].chroma_offset[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[0],
                                    roi_cr);
                            }
                            else if (weighted_bipred_idc == 2)
                            {
                                // combine bidir predictions into one, implicit weighting
                                Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;
                                // luma
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi);
                                // chroma
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                    pTmpV/*pDstV_sb*/,
                                    pDstV_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi_cr);
                                ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                    pTmpU/*pDstU_sb*/,
                                    pDstU_sb/*-8*/,
                                    pitch,
                                    nTmpPitch,
                                    pitch,
                                    64 - iDistScaleFactor,
                                    iDistScaleFactor,
                                    roi_cr);
                            }
                            else
                                VM_ASSERT(0);
                        }    // weighted
                    }    // LoopCnt > 1
                    block++;
                }    // xpos
            }    // ypos
        }    // not 8x8
        else
        {
            // MBTYPE_INTER_8x8:
            // 4 8x8 subblocks, each of which can be a different mode.
            for (block=0; block<4; block++)
            {
                {
                    blocktype = psbtype[block];
                }
                switch (blocktype)
                {
                case SBTYPE_8x8:
                    roi.width        = 8;
                    roi.height       = 8;
                    roi_cr.width     = 8;
                    roi_cr.height    = 8;
                    break;
                case SBTYPE_8x4:
                    roi.width        = 8;
                    roi.height       = 4;
                    roi_cr.width     = 8;
                    roi_cr.height    = 4;
                    break;
                case SBTYPE_4x8:
                    roi.width        = 4;
                    roi.height       = 8;
                    roi_cr.width     = 4;
                    roi_cr.height    = 8;
                    break;
                case SBTYPE_DIRECT:        // for spatial mode DIRECT
                    roi.width        = 4<<(Ipp8u)bUseDirect8x8Inference;
                    roi.height       = 4<<(Ipp8u)bUseDirect8x8Inference;
                    roi_cr.width     = (4<<(Ipp8u)bUseDirect8x8Inference);
                    roi_cr.height    = (4<<(Ipp8u)bUseDirect8x8Inference);
                    break;
                case SBTYPE_4x4:
                    roi.width        = 4;
                    roi.height       = 4;
                    roi_cr.width     = 4;
                    roi_cr.height    = 4;
                    break;
                default:
                    VM_ASSERT(0);
                    return;
                }    // switch sbtype

                for (ypos=yoff8[block]; ypos<yoff8[block]+8; ypos+=roi.height)
                {
                    for (xpos=xoff8[block]; xpos<xoff8[block]+8; xpos+=roi.width)
                    {
                        if ((psbdir[block] == D_DIR_BIDIR) ||
                            (psbdir[block] == D_DIR_DIRECT) ||
                            (psbdir[block] == D_DIR_DIRECT_SPATIAL_BIDIR))
                        {
                            uBlockDir = D_DIR_BIDIR;
                            loopCnt = 2;
                            bUnidirWeightSB =  false;
                        }
                        else
                        {
                            loopCnt = 1;
                            if ((psbdir[block] == D_DIR_BWD) ||
                                (psbdir[block] == D_DIR_DIRECT_SPATIAL_BWD))
                                uBlockDir = D_DIR_BWD;
                            else
                                uBlockDir = D_DIR_FWD;
                            bUnidirWeightSB =  bUnidirWeightMB;
                        }

                        for (i=0; i<loopCnt; i++)
                        {
                            if ((uBlockDir == D_DIR_BWD) || (i > 0))
                            {
                                pRefIndex = pRefIndexL1 + (xpos>>2) + (ypos>>2)*4;
                                RefIndexL1 = *pRefIndex;

                                VM_ASSERT(RefIndexL1 >= 0 && RefIndexL1 <
                                    (Ipp8s)m_pSliceHeader->num_ref_idx_l1_active);

								//if( pRefPicList1[RefIndexL1] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
								//{//***bnie: 
								//	return;
								//}

                                pRefYPlane = pRefPicList1[RefIndexL1]->m_pYPlane;
                                pRefVPlane = pRefPicList1[RefIndexL1]->m_pVPlane;
                                pRefUPlane = pRefPicList1[RefIndexL1]->m_pUPlane;

                                VM_ASSERT(pRefYPlane);
                                VM_ASSERT(pRefVPlane);
                                VM_ASSERT(pRefUPlane);

                                pMV = pMVBwd;
                            }
                            else
                            {
                                pRefIndex = pRefIndexL0 + (xpos>>2) + (ypos>>2)*4;
                                RefIndexL0 = *pRefIndex;


                                VM_ASSERT(RefIndexL0 >= 0 && RefIndexL0 <
                                    (Ipp8s)m_pSliceHeader->num_ref_idx_l0_active);

 								//if( pRefPicList0[RefIndexL0] == (H264VideoDecoder::H264DecoderFrame *)0xffffffff )
								//{//***bnie: 
								//	return;
								//}

                                pRefYPlane = pRefPicList0[RefIndexL0]->m_pYPlane;
                                pRefVPlane = pRefPicList0[RefIndexL0]->m_pVPlane;
                                pRefUPlane = pRefPicList0[RefIndexL0]->m_pUPlane;

                                VM_ASSERT(pRefYPlane);
                                VM_ASSERT(pRefVPlane);
                                VM_ASSERT(pRefUPlane);

                                pMV = pMVFwd;
                            }
                            // set pointers for this subblock
                            pMV_sb = pMV + (xpos>>2) + (ypos>>2)*4;

                            mvx = pMV_sb->mvx;
                            mvy = pMV_sb->mvy;

                            //offsetToBlock = xpos + ypos*pitch;
                            pDstY_sb = pDstY + xpos + ypos*pitch;//offsetToBlock;
                            pRefY_sb = pRefYPlane + xpos + ypos*pitch;//offsetToBlock;
                            pDstV_sb = pDstV + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);
                            pRefV_sb = pRefVPlane + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);
                            pDstU_sb = pDstU + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);
                            pRefU_sb = pRefUPlane + (xpos) + (ypos)*pitch;//(offsetToBlock>>1);

                            if (i > 0)
                            {/*
                             // advance Dst ptrs to next MB position, used as temp store
                             // for backward prediction. This is always OK because the Dst
                             // buffer is padded at the edges.
                             pDstY_sb += 16;
                             pDstV_sb += 8;
                             pDstU_sb += 8;*/

                                pTmpY = align_pointer<Ipp8u *> (TemporaryBufferForMotionPrediction, DEFAULT_ALIGN_VALUE);
                                pTmpU = pTmpY + 16 * 16;
                                pTmpV = pTmpU + 16 * 16;
                                nTmpPitch = 16;
                            }
                            else
                            {
                                pTmpY = pDstY_sb;
                                pTmpU = pDstU_sb;
                                pTmpV = pDstV_sb;
                                nTmpPitch = pitch;
                            }

                            xh = mvx & (INTERP_FACTOR-1);
                            yh = mvy & (INTERP_FACTOR-1);

                            Ipp8u pred_method=SelectPredictionMethod(
                                mbYOffset+ypos,
                                mvy,
                                roi.height,
                                height);
                            // See comment above about clipping & xh,yh.
                            mvx = MIN(mvx, (width - ((Ipp32s)mbXOffset + xpos + roi.width - 1
                                - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvx = MAX(mvx, -((Ipp32s)(mbXOffset + xpos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));
                            mvy = MIN(mvy, (height - ((Ipp32s)mbYOffset + ypos + roi.height -
                                1 - D_MV_CLIP_LIMIT))*INTERP_FACTOR);
                            mvy = MAX(mvy, -((Ipp32s)(mbYOffset + ypos + D_MV_CLIP_LIMIT)*INTERP_FACTOR));

                            mvyc = mvy;

                            xint = mvx >> INTERP_SHIFT;
                            yint = mvy >> INTERP_SHIFT;

                            switch(pred_method)
                            {
                            case ALLOK:
                                pRef = pRefY_sb + offsetY + xint + yint * pitch;

                                this->m_ippiInterpolateLuma_H264_8u_C1R(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, roi);

                                break;
                            case PREDICTION_FROM_TOP:
                                pRef = pRefY_sb + offsetY + xint + yint * pitch;

                                ippiInterpolateLumaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, - ((Ipp32s)mbYOffset+ypos+yint),roi);
                                break;
                            case PREDICTION_FROM_BOTTOM:
                                pRef = pRefY_sb + offsetY + xint + yint * pitch;

                                ippiInterpolateLumaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpY/*pDstY_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh, ((Ipp32s)mbYOffset+ypos+yint+roi.height)-height,roi);
                                break;

                            default:VM_ASSERT(0);
                                break;
                            }

                            // optional prediction weighting
                            if (bUnidirWeightSB &&
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_weight_flag != 0)
                            {
                                ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpY/*pDstY_sb*/,
                                    nTmpPitch/*pitch*/,
                                    luma_log2_weight_denom,
                                    m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                                    m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                                    roi);
                                /*
                                ippiWeightedAverage_H264_8u_C1IR( pDstY_sb,pDstY_sb,pitch,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_weight,
                                0,
                                luma_log2_weight_denom,
                                m_pPredWeight[uBlockDir][*pRefIndex].luma_offset,
                                roi);*/

                            }
                            // chroma (1/8 pixel MV)
                            xh = (mvx &  (((INTERP_FACTOR*2)>>1)-1))<<1;
                            yh = (mvyc & (((INTERP_FACTOR*2)>>1)-1))<<1;
                            xint = mvx >>  (INTERP_SHIFT);
                            yint = mvyc >> (INTERP_SHIFT);

                            switch(pred_method)
                            {
                            case ALLOK:
                                pRef = pRefV_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   roi_cr);

                                pRef = pRefU_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChroma_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   roi_cr);
                                break;

                            case PREDICTION_FROM_TOP:
                                pRef = pRefV_sb + offsetC + xint + yint * pitch;


                                ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,  - ((((Ipp32s)mbYOffset+ypos))+yint), roi_cr);


                                pRef = pRefU_sb + (offsetY>>1) + xint + yint * pitch;

                                ippiInterpolateChromaTop_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,  - ((((Ipp32s)mbYOffset+ypos))+yint), roi_cr);

                                break;
                            case PREDICTION_FROM_BOTTOM:
                                pRef = pRefV_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpV/*pDstV_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);

                                pRef = pRefU_sb + offsetC + xint + yint * pitch;

                                ippiInterpolateChromaBottom_H264_8u_C1R_x(pRef, pitch,
                                    pTmpU/*pDstU_sb*/, nTmpPitch/*pitch*/,
                                    xh, yh,   ((((Ipp32s)mbYOffset+ypos))+yint+roi_cr.height)-(height),roi_cr);
                                break;
                            default:VM_ASSERT(0);
                                break;
                            }
                            // optional prediction weighting
                            if (bUnidirWeightSB &&
                                m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight_flag != 0)
                            {
                                ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpV/*pDstV_sb*/,
                                    nTmpPitch/*pitch*/,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                                    roi_cr);
                                ippiUniDirWeightBlock_H264_8u_C1R_x(pTmpU/*pDstU_sb*/,
                                    nTmpPitch/*pitch*/,
                                    chroma_log2_weight_denom,
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[0],
                                    m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[0],
                                    roi_cr);
                                //ippiWeightedAverage_H264_8u_C1IR( pDstV_sb,pDstV_sb,pitch,
                                //                                       m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[1],
                                //                                      0,
                                //                                      chroma_log2_weight_denom,
                                //                                      m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[1],
                                //                                      roi_cr);

                                //ippiWeightedAverage_H264_8u_C1IR( pDstU_sb,pDstU_sb,pitch,
                                //                                       m_pPredWeight[uBlockDir][*pRefIndex].chroma_weight[0],
                                //                                       0,
                                //                                       chroma_log2_weight_denom,
                                //                                       m_pPredWeight[uBlockDir][*pRefIndex].chroma_offset[0],
                                //                                       roi_cr);

                            }
                        }    // loopCnt
                        if (loopCnt > 1)
                        {
                            if (!bBidirWeightMB)
                            {
                                // combine bidir predictions into one, no weighting

                                // luma
                                //                            (*m_InterpolBlock)(pDstY_sb-16, pDstY_sb, pDstY_sb-16, roi.width,
                                //                                            roi.height, pitch, pitch);
                                // chroma
                                //                            (*m_InterpolBlock)(pDstV_sb-8, pDstV_sb, pDstV_sb-8, roi.width>>1,
                                //                                            roi.height>>1, pitch, pitch);
                                //                            (*m_InterpolBlock)(pDstU_sb-8, pDstU_sb, pDstU_sb-8, roi.width>>1,
                                //                                            roi.height>>1, pitch, pitch);
                                ippiInterpolateBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                    pTmpY/*pDstY_sb*/,
                                    pDstY_sb/*-16*/,
                                    roi.width,
                                    roi.height,
                                    pitch,
                                    nTmpPitch,
                                    pitch);
                                ippiInterpolateBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                    pTmpV/*pDstV_sb*/,
                                    pDstV_sb/*-8*/,
                                    roi_cr.width,
                                    roi_cr.height,
                                    pitch,
                                    nTmpPitch,
                                    pitch);
                                ippiInterpolateBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                    pTmpU/*pDstU_sb*/,
                                    pDstU_sb/*-8*/,
                                    roi_cr.width,
                                    roi_cr.height,
                                    pitch,
                                    nTmpPitch,
                                    pitch);
                            }
                            else
                            {
                                // combine bidir predictions into one with weighting
                                // combine bidir predictions into one with weighting
                                if (weighted_bipred_idc == 1)
                                {
                                    // combine bidir predictions into one, explicit weighting

                                    // luma
                                    /*
                                    ippiWeightedAverage_H264_8u_C1IR( pDstY_sb-16,pDstY_sb,pitch,
                                    m_pPredWeight[0][RefIndexL0].luma_weight,
                                    m_pPredWeight[1][RefIndexL1].luma_weight,
                                    luma_log2_weight_denom,
                                    (m_pPredWeight[0][RefIndexL0].luma_offset+
                                    m_pPredWeight[1][RefIndexL1].luma_offset+1)>>1,
                                    roi);
                                    ippiWeightedAverage_H264_8u_C1IR( pDstV_sb-8,pDstY_sb,pitch,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                    chroma_log2_weight_denom,
                                    (m_pPredWeight[0][RefIndexL0].chroma_offset[1]+
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[1]+1)>>1,
                                    roi_cr);
                                    ippiWeightedAverage_H264_8u_C1IR( pDstU_sb-8,pDstU_sb,pitch,
                                    m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                    m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                    chroma_log2_weight_denom,
                                    (m_pPredWeight[0][RefIndexL0].chroma_offset[0]+
                                    m_pPredWeight[1][RefIndexL1].chroma_offset[0]+1)>>1,
                                    roi_cr);*/
                                    ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                        pTmpY/*pDstY_sb*/,
                                        pDstY_sb/*-16*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        luma_log2_weight_denom,
                                        m_pPredWeight[0][RefIndexL0].luma_weight,
                                        m_pPredWeight[0][RefIndexL0].luma_offset,
                                        m_pPredWeight[1][RefIndexL1].luma_weight,
                                        m_pPredWeight[1][RefIndexL1].luma_offset,
                                        roi);
                                    // chroma
                                    ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                        pTmpV/*pDstV_sb*/,
                                        pDstV_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        chroma_log2_weight_denom,
                                        m_pPredWeight[0][RefIndexL0].chroma_weight[1],
                                        m_pPredWeight[0][RefIndexL0].chroma_offset[1],
                                        m_pPredWeight[1][RefIndexL1].chroma_weight[1],
                                        m_pPredWeight[1][RefIndexL1].chroma_offset[1],
                                        roi_cr);
                                    ippiBiDirWeightBlock_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                        pTmpU/*pDstU_sb*/,
                                        pDstU_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        chroma_log2_weight_denom,
                                        m_pPredWeight[0][RefIndexL0].chroma_weight[0],
                                        m_pPredWeight[0][RefIndexL0].chroma_offset[0],
                                        m_pPredWeight[1][RefIndexL1].chroma_weight[0],
                                        m_pPredWeight[1][RefIndexL1].chroma_offset[0],
                                        roi_cr);
                                }
                                else if (weighted_bipred_idc == 2)
                                {
                                    // combine bidir predictions into one, implicit weighting
                                    Ipp32s iDistScaleFactor = pDistScaleFactors[RefIndexL0]>>2;

                                    // luma
                                    ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstY_sb/*-16*/,
                                        pTmpY/*pDstY_sb*/,
                                        pDstY_sb/*-16*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        64 - iDistScaleFactor,
                                        iDistScaleFactor,
                                        roi);
                                    // chroma
                                    ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstV_sb/*-8*/,
                                        pTmpV/*pDstV_sb*/,
                                        pDstV_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        64 - iDistScaleFactor,
                                        iDistScaleFactor,
                                        roi_cr);
                                    ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x(pDstU_sb/*-8*/,
                                        pTmpU/*pDstU_sb*/,
                                        pDstU_sb/*-8*/,
                                        pitch,
                                        nTmpPitch,
                                        pitch,
                                        64 - iDistScaleFactor,
                                        iDistScaleFactor,
                                        roi_cr);
                                }
                                else
                                    VM_ASSERT(0);
                            }    //  weighted
                        }    // LoopCnt >1
                    }    // for xpos
                }    // for ypos
            }    // for block
        }    // 8x8

    } // void H264SegmentDecoder::ReconstructMacroblock(Ipp8u *pDstY,

////////////////////////////////////////////////////////////////////////////////
// Copy raw pixel values from the bitstream to the reconstructed frame for
// all luma and chroma blocks of one macroblock.
////////////////////////////////////////////////////////////////////////////////
void H264SegmentDecoder::ReconstructPCMMB(Ipp32u lumaOffset,Ipp32u chromaOffset,Ipp8u color_format)
{
    Ipp8u *pDstY;
    Ipp8u *pDstU;
    Ipp8u *pDstV;
    Ipp32u pitch;
    Ipp32s i;
    Ipp8u *pCoeffBlocksRead = reinterpret_cast<Ipp8u *> (m_pCoeffBlocksRead);

    // to retrieve non-aligned pointer from m_pCoeffBlocksRead
    pitch = m_pCurrentFrame->pitch();//***bnie: <<(Ipp32u) (m_pCurrentFrame->m_PictureStructureForDec<FRM_STRUCTURE);
    pDstY = m_pCurrentFrame->m_pYPlane + lumaOffset;
    pDstU = m_pCurrentFrame->m_pUPlane + chromaOffset;
    pDstV = m_pCurrentFrame->m_pVPlane + chromaOffset;

    // get pointer to raw bytes from m_pCoeffBlocksRead
    for (i = 0; i<16; i++)
        memcpy(pDstY + i * pitch, pCoeffBlocksRead + i * 16, 16);
    pCoeffBlocksRead += 16 * 16;

    if (color_format)
    {
        Ipp32s iWidth = (color_format == 3) ? (16) : (8);
        Ipp32s iHeight = 8 + 8 * (color_format >> 1);

        for (i = 0; i < iHeight; i += 1)
            memcpy(pDstU + i * pitch, pCoeffBlocksRead + i * iWidth, iWidth);
        pCoeffBlocksRead += iWidth * iHeight;
        for (i = 0; i < iHeight; i += 1)
            memcpy(pDstV + i * pitch, pCoeffBlocksRead + i * iWidth, iWidth);
        pCoeffBlocksRead += iWidth * iHeight;
    }

    m_pCoeffBlocksRead = (Ipp16s *) pCoeffBlocksRead;

} // void H264SegmentDecoder::ReconstructPCMMB(Ipp32u lumaOffset,Ipp8u color_format)

} // namespace UMC
