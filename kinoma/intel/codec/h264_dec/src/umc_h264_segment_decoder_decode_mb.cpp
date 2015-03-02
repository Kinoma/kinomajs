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

// 0 1 4 5
// 2 3 6 7
// 8 9 c d
// a b e f

const
Ipp8u block2lin[16] =
{
     0, 1, 4, 5,
     2, 3, 6, 7,
     8, 9,12,13,
     10,11,14,15
};

void H264SegmentDecoder::ComputeMotionVectorPredictors(const Ipp8u ListNum,
                                                       Ipp8s RefIndex, // reference index for this part
                                                       const Ipp32s block, // block or subblock number, depending on mbtype
                                                       Ipp32s *pMVx, // resulting MV predictors
                                                       Ipp32s *pMVy)
{
    Ipp32s     px0, px1, px2;
    Ipp32s     py0, py1, py2;
    //    Ipp32s     diff;
    Ipp32s  isRightUnavailable=false;
    // Indicates whether the (above) right block, subblock or
    // macroblock can be used for motion vector prediction.
    // This is not a true boolean, in that we use bitwise operations
    // so that any non-zero value, not just the value true,
    // is considered true.
    Ipp32s  isLeftUnavailable=false;
    // Indicates whether the (above) left block, subblock or
    // macroblock can be used for motion vector prediction.
    // Only used when isRightUnavailable is non-zero.

    H264DecoderMotionVector *sl; // pointer to sb to the left
    H264DecoderMotionVector *sa; // pointer to sb above
    H264DecoderMotionVector *sr; // pointer to sb above right
    Ipp8s *pRefIxl;                    // pointer to corresponding RefIndex sb
    Ipp8s *pRefIxa;
    Ipp8s *pRefIxr;
    H264DecoderMotionVector *sonly = NULL; // pointer to only MV this ref
    H264DecoderMotionVector null = {0,0};
    Ipp8s nullRefIx = -1;
    Ipp32u uSameRefPicCount = 3;
    // To code the rule that if only one of the three reference MV is to
    // the same reference picture as the MV being computed, then that one
    // MV is used as the MV predictor. Initialize to all same, then decrement
    // as "different reference picture" are found.
    H264DecoderBlockLocation Left={-1,0},Top={-1,0},TopRight={-1,0},TopLeft={-1,0};
    H264DecoderMacroblockMVs *MVs;
    H264DecoderMacroblockRefIdxs *RefIdxs;
    H264DecoderMacroblockGlobalInfo *gmbs;
    MVs = m_pCurrentFrame->m_mbinfo.MV[ListNum];
    RefIdxs = m_pCurrentFrame->m_mbinfo.RefIdxs[ListNum];
    gmbs = m_pCurrentFrame->m_mbinfo.mbs;

    {
        switch (m_cur_mb.GlobalMacroblockInfo->mbtype)
        {
        case MBTYPE_FORWARD:
        case MBTYPE_BACKWARD:
        case MBTYPE_BIDIR:
        case MBTYPE_SKIPPED:
            VM_ASSERT(block == 0);

            Left = m_cur_mb.CurrentBlockNeighbours.mbs_left[0];
            Top = m_cur_mb.CurrentBlockNeighbours.mb_above;
            TopRight = m_cur_mb.CurrentBlockNeighbours.mb_above_right;
            TopLeft= m_cur_mb.CurrentBlockNeighbours.mb_above_left;

            isRightUnavailable = (TopRight.mb_num<0);
            isLeftUnavailable = (TopLeft.mb_num<0);

            break;
        case MBTYPE_INTER_16x8:
            VM_ASSERT(block >= 0 && block <= 1);
            // First check for availability of directional predictor which is
            // just used if available.
            if (block == 0)
            {
                // upper half, use predictor from above
                Top= m_cur_mb.CurrentBlockNeighbours.mb_above;
                if (Top.mb_num>=0)
                {

                    if (IS_INTER_MBTYPE(gmbs[Top.mb_num].mbtype) &&
                        (((RefIdxs[Top.mb_num].RefIdxs[Top.block_num])) == RefIndex))
                    {
                        *pMVx = MVs[Top.mb_num].MotionVectors[Top.block_num].mvx;
                        *pMVy = MVs[Top.mb_num].MotionVectors[Top.block_num].mvy;
                        goto done;
                    }
                    else
                    {
                        goto median16x8_0;
                    }
                }
                else
                {
median16x8_0:
                    Left = m_cur_mb.CurrentBlockNeighbours.mbs_left[0];
                    TopRight = m_cur_mb.CurrentBlockNeighbours.mb_above_right;
                    TopLeft = m_cur_mb.CurrentBlockNeighbours.mb_above_left;

                    // init vars for median prediction
                    isRightUnavailable = (TopRight.mb_num<0);
                    if (isRightUnavailable)
                        isLeftUnavailable = (TopLeft.mb_num<0);
                }
            }
            else
            {
                Left = m_cur_mb.CurrentBlockNeighbours.mbs_left[2];
                // lower half, use predictor from left
                if ( Left.mb_num>=0)
                {

                    if (IS_INTER_MBTYPE(gmbs[Left.mb_num].mbtype) &&
                        (((RefIdxs[Left.mb_num].RefIdxs[Left.block_num])) == RefIndex))
                    {
                        *pMVx = MVs[Left.mb_num].MotionVectors[Left.block_num].mvx;
                        *pMVy = MVs[Left.mb_num].MotionVectors[Left.block_num].mvy;
                        goto done;
                    }
                    else
                    {
                        goto median_16x8_1;
                    }
                }
                else
                {
median_16x8_1:

                    Top.mb_num = m_CurMBAddr;
                    Top.block_num = 4;

                    TopLeft.block_num = 8;
                    GetTopLeftLocationForCurrentMBLumaNonMBAFF(&TopLeft);

                    // init vars for median prediction
                    isRightUnavailable = 1;
                    isLeftUnavailable = (Left.mb_num<0);
                }
            }
            break;
        case MBTYPE_INTER_8x16:
            VM_ASSERT(block >= 0 && block <= 1);
            // First check for availability of directional predictor which is
            // just used if available.
            if (block == 0)
            {
                // left half, use predictor from left
                //LeftBlockNum = block;
                //LeftMB=GetLeftBlock(pMBInfo,LeftBlockNum);
                Left = m_cur_mb.CurrentBlockNeighbours.mbs_left[0];
                if (Left.mb_num>=0)
                {
                    if (IS_INTER_MBTYPE(gmbs[Left.mb_num].mbtype) &&
                        (((RefIdxs[Left.mb_num].RefIdxs[Left.block_num])) == RefIndex))
                    {
                        *pMVx = MVs[Left.mb_num].MotionVectors[Left.block_num].mvx;
                        *pMVy = MVs[Left.mb_num].MotionVectors[Left.block_num].mvy;
                        goto done;
                    }
                    else
                    {
                        goto median_8x16_0;
                    }
                }
                else
                {
median_8x16_0:
                    /*                TopBlockNum=0;
                    TopLeftBlockNum=0;
                    TopRightBlockNum=1;

                    TopMB=GetTopBlock(pMBInfo,TopBlockNum);
                    TopLeftMB=GetTopLeftBlock(pMBInfo,TopLeftBlockNum);
                    TopRightMB=GetTopRightBlock(pMBInfo,TopRightBlockNum);*/

                    Top = m_cur_mb.CurrentBlockNeighbours.mb_above;
                    TopRight = m_cur_mb.CurrentBlockNeighbours.mb_above;
                    TopLeft= m_cur_mb.CurrentBlockNeighbours.mb_above_left;
                    TopRight.block_num+=2;
                    // init vars for median prediction
                    isRightUnavailable = (Top.mb_num<0);
                    if (isRightUnavailable)
                        isLeftUnavailable = (TopLeft.mb_num<0);
                }
            }
            else
            {
                // right half, use predictor from above right unless unavailable,
                // then try above left
                //TopRightBlockNum=3;
                //TopRightMB=GetTopRightBlock(pMBInfo,TopRightBlockNum);
                //TopBlockNum=2;
                //TopMB=GetTopBlock(pMBInfo,TopBlockNum);
                TopRight= m_cur_mb.CurrentBlockNeighbours.mb_above_right;
                Top= m_cur_mb.CurrentBlockNeighbours.mb_above;
                Top.block_num+=2;

                if ( TopRight.mb_num>=0)
                {
                    if (IS_INTER_MBTYPE(gmbs[TopRight.mb_num].mbtype) &&
                        (((RefIdxs[TopRight.mb_num].RefIdxs[TopRight.block_num])) == RefIndex))
                    {
                        *pMVx = MVs[TopRight.mb_num].MotionVectors[TopRight.block_num].mvx;
                        *pMVy = MVs[TopRight.mb_num].MotionVectors[TopRight.block_num].mvy;
                        goto done;
                    }
                }
                else  if ( Top.mb_num>=0)
                {
                    if (IS_INTER_MBTYPE(gmbs[Top.mb_num].mbtype) &&
                        (((RefIdxs[Top.mb_num].RefIdxs[Top.block_num-1])) == RefIndex))
                    {
                        *pMVx = MVs[Top.mb_num].MotionVectors[Top.block_num-1].mvx;
                        *pMVy = MVs[Top.mb_num].MotionVectors[Top.block_num-1].mvy;
                        goto done;
                    }
                }


                //LeftBlockNum=2;
                //LeftMB=GetLeftBlock(pMBInfo,LeftBlockNum);
                Left.mb_num=m_CurMBAddr;
                Left.block_num=1;
                TopLeft=m_cur_mb.CurrentBlockNeighbours.mb_above;
                TopLeft.block_num++;
                // init vars for median prediction
                isRightUnavailable = (TopRight.mb_num<0);
                if (isRightUnavailable)
                    isLeftUnavailable = (Top.mb_num<0);
            }
            //        diff = 2;
            break;
        case MBTYPE_INTER_8x8:
        case MBTYPE_INTER_8x8_REF0:
            {
                // Each 8x8 block of a macroblock can be subdivided into subblocks,
                // each having its own MV. The parameter 'block' has block and
                // subblock information:
                //  block 0..3, bits 2-3
                //  subblock 0..3, bits 0-1
                Ipp32s  left_edge_block = 0, top_edge_block = 0, right_edge_block = 0;

                switch (m_cur_mb.GlobalMacroblockInfo->sbtype[block>>2])
                {
                case SBTYPE_8x8:
                    Top.block_num=
                        Left.block_num=
                        TopLeft.block_num=
                        block2lin[block];
                    TopRight.block_num=
                        block2lin[block]+1;
                        GetLeftLocationForCurrentMBLumaNonMBAFF(&Left);
                        GetTopLocationForCurrentMBLumaNonMBAFF(&Top);
                        GetTopRightLocationForCurrentMBLumaNonMBAFF(&TopRight);
                        GetTopLeftLocationForCurrentMBLumaNonMBAFF(&TopLeft);
                    switch (block>>2)
                    {
                    case 0:
                        left_edge_block = 1;
                        top_edge_block = 1;
                        isRightUnavailable = (Top.mb_num<0);
                        isLeftUnavailable = (TopLeft.mb_num<0);
                        break;
                    case 1:
                        left_edge_block = 0;
                        top_edge_block = 1;
                        isRightUnavailable = (TopRight.mb_num<0);
                        isLeftUnavailable = (Top.mb_num<0);
                        break;
                    case 2:
                        left_edge_block = 1;
                        top_edge_block = 0;
                        isRightUnavailable = 0;
                        break;
                    case 3:
                        left_edge_block = 0;
                        top_edge_block = 0;
                        isRightUnavailable = 1;
                        isLeftUnavailable = 0;
                        break;
                    }   // block
                    right_edge_block = left_edge_block == 0 ? 1 : 0;
                    break;
                case SBTYPE_8x4:
                    Top.block_num=
                        Left.block_num=
                        TopLeft.block_num=
                        block2lin[block&(-4)]+4*(block&1);
                    TopRight.block_num=
                        block2lin[block&(-4)]+4*(block&1)+1;

                        GetLeftLocationForCurrentMBLumaNonMBAFF(&Left);
                        GetTopLocationForCurrentMBLumaNonMBAFF(&Top);
                        GetTopRightLocationForCurrentMBLumaNonMBAFF(&TopRight);
                        GetTopLeftLocationForCurrentMBLumaNonMBAFF(&TopLeft);

                    left_edge_block = left_edge_tab16_8x4[block];
                    top_edge_block = top_edge_tab16_8x4[block];
                    right_edge_block = right_edge_tab16_8x4[block];

                    if (!top_edge_block)
                    {
                        isRightUnavailable = (above_right_avail_8x4[block] == 0);
                        if (isRightUnavailable)
                            isLeftUnavailable = left_edge_block ? (Left.mb_num<0) : 0;
                    }
                    else
                    {
                        // top edge of 8x4
                        if (!right_edge_block)
                        {
                            isRightUnavailable = (Top.mb_num<0);
                            isLeftUnavailable = (TopLeft.mb_num<0);
                        }
                        else
                        {
                            isRightUnavailable = (TopRight.mb_num<0);
                            isLeftUnavailable = (Top.mb_num<0);
                        }
                    }
                    break;
                case SBTYPE_4x8:
                    Top.block_num=
                        Left.block_num=
                        TopLeft.block_num=
                        block2lin[block&(-4)]+(block&1);
                    TopRight.block_num=
                        block2lin[block&(-4)]+(block&1);

                        GetLeftLocationForCurrentMBLumaNonMBAFF(&Left);
                        GetTopLocationForCurrentMBLumaNonMBAFF(&Top);
                        GetTopRightLocationForCurrentMBLumaNonMBAFF(&TopRight);
                        GetTopLeftLocationForCurrentMBLumaNonMBAFF(&TopLeft);
                    //            diff = 1;
                    left_edge_block = left_edge_tab16_4x8[block];
                    top_edge_block = top_edge_tab16_4x8[block];
                    right_edge_block = right_edge_tab16_4x8[block];
                    if (!top_edge_block)
                    {
                        isRightUnavailable = (above_right_avail_4x8[block] == 0);
                        isLeftUnavailable = 0;  // always, when above right not available
                    }
                    else
                    {
                        if (!right_edge_block)
                            isRightUnavailable = (Top.mb_num<0);
                        else
                            isRightUnavailable = (TopRight.mb_num<0);
                        if (isRightUnavailable)
                        {
                            if (!left_edge_block)
                                isLeftUnavailable = (Top.mb_num<0);
                            else
                                isLeftUnavailable = (TopLeft.mb_num<0);
                        }
                    }
                    break;
                case SBTYPE_4x4:
                    Top.block_num=
                        Left.block_num=
                        TopLeft.block_num=
                        TopRight.block_num=
                        block2lin[block];

                        GetLeftLocationForCurrentMBLumaNonMBAFF(&Left);
                        GetTopLocationForCurrentMBLumaNonMBAFF(&Top);
                        GetTopRightLocationForCurrentMBLumaNonMBAFF(&TopRight);
                        GetTopLeftLocationForCurrentMBLumaNonMBAFF(&TopLeft);
                    VM_ASSERT(block >= 0 && block <= 15);
                    //            diff = 1;
                    left_edge_block = left_edge_tab16[block];
                    top_edge_block = top_edge_tab16[block];
                    right_edge_block = right_edge_tab16[block];
                    if (!top_edge_block)
                        isRightUnavailable = (above_right_avail_4x4[block] == 0);
                    else
                    {
                        if (!right_edge_block)
                            isRightUnavailable = (Top.mb_num<0);
                        else
                            isRightUnavailable = (TopRight.mb_num<0);
                    }
                    if (isRightUnavailable)
                    {
                        // When not on top edge of MB, for blocks for which right may not
                        // be available, left is always available.
                        if (top_edge_block == 0)
                            isLeftUnavailable = 0;
                        else
                        {
                            if (!left_edge_block)
                                isLeftUnavailable = (Top.mb_num<0);
                            else
                                isLeftUnavailable = (TopLeft.mb_num<0);
                        }
                    }
                    break;
                }
            }

            break;
        default:
            *pMVx = 0;
            *pMVy = 0;
            goto done;
        }

        // correct for left edge
        if (Left.mb_num<0)
        {
            sl = &null;
            pRefIxl = &nullRefIx;
        }
        else
        {
            sl = &MVs[Left.mb_num].MotionVectors[Left.block_num];
            pRefIxl = &RefIdxs[Left.mb_num].RefIdxs[Left.block_num];
        }

        // correct for top edge
        if (Top.mb_num<0)
        {
            sa = &null;
            pRefIxa = &nullRefIx;
        }
        else
        {
            sa = &MVs[Top.mb_num].MotionVectors[Top.block_num];
            pRefIxa = &RefIdxs[Top.mb_num].RefIdxs[Top.block_num];

        }

        // correct for upper right
        if (isRightUnavailable)
        {
            // replace with block above left if available
            if (isLeftUnavailable)
            {
                sr = &null;
                pRefIxr = &nullRefIx;
            }
            else
            {
                sr = &MVs[TopLeft.mb_num].MotionVectors[TopLeft.block_num];
                pRefIxr = &RefIdxs[TopLeft.mb_num].RefIdxs[TopLeft.block_num];
            }
        }
        else
        {
            sr = &MVs[TopRight.mb_num].MotionVectors[TopRight.block_num];
            pRefIxr = &RefIdxs[TopRight.mb_num].RefIdxs[TopRight.block_num];
        }

        // If above AND right are unavailable AND left is available, use left
        if ((sa == &null) && (sr == &null) && (sl != &null))
        {
            // return MV at sl
            *pMVx = sl->mvx;
            *pMVy = sl->mvy;
        }
        else
        {
            // Check for more than one predictor from different reference frame
            // If there is only one predictor from this ref frame, then sonly will
            // be pointing to it.
            if ((*pRefIxl) != RefIndex)
                uSameRefPicCount--;
            else
            {
                sonly = sl;
            }
            if ((*pRefIxa) != RefIndex)
                uSameRefPicCount--;
            else
            {
                sonly = sa;
            }
            if ((*pRefIxr) != RefIndex)
                uSameRefPicCount--;
            else
            {
                sonly = sr;
            }

            if (uSameRefPicCount != 1)
            {
                // "normal" median prediction
                px0 = sl->mvx;
                px1 = sa->mvx;
                px2 = sr->mvx;

#define MEDIAN_OF_3(a, b, c) (MIN((a),(b))) ^ (MIN((b),(c))) ^ (MIN((c),(a)))

                *pMVx = MEDIAN_OF_3(px0, px1, px2);

                py0 = sl->mvy;
                py1 = sa->mvy;
                py2 = sr->mvy;

                *pMVy = MEDIAN_OF_3(py0, py1, py2);
            }
            else
            {
                // return MV at sonly
                *pMVx = sonly->mvx;
                *pMVy = sonly->mvy;
            }
        }

    }

done:
#ifdef STORE_PREDICTORS
    if (mvs==NULL) mvs=fopen(__PREDICTORS_FILE__,"w+t");
    if (mvs)
    {
        char slice[]={'P','B','I'};
        fprintf(mvs,"%d %d,%d %d %c\n",numvecs++,*pMVx,*pMVy,MBAddr2MBCount(m_CurMBAddr),slice[m_pSliceHeader->slice_type]);
    }
#endif

    return;

} // void H264SegmentDecoder::ComputeMotionVectorPredictors(const Ipp8u ListNum,

Status H264SegmentDecoder::DecodeMotionVectors(bool bIsBSlice)
{
    Ipp32s num_vectorsL0, block, subblock, mvdx, mvdy, pmvx, pmvy, i, j;
    Ipp32s num_vectorsL1;
    Ipp32s dec_vectorsL0;
    Ipp32s dec_vectorsL1;
    Ipp32s num_refIxL0;
    Ipp32s num_refIxL1;
    Ipp32s dec_refIxL0;
    Ipp32s dec_refIxL1;
    Ipp32u uVLCCodes[64+MAX_NUM_REF_FRAMES*2];        // sized for max possible:
                                                //  16 MV * 2 codes per MV * 2 directions
                                                //  plus L0 and L1 ref indexes
    Ipp32u *pVLCCodes;        // pointer to pass to VLC function
    Ipp32u *pVLCCodesL0MV;
    Ipp32u *pVLCCodesL1MV;
    Ipp32u *pVLCCodesL0RefIndex;
    Ipp32u *pVLCCodesL1RefIndex;
    Ipp32s  mvxL0 = 0, mvyL0 = 0;
    Ipp32s  mvxL1 = 0, mvyL1 = 0;
//    Ipp32u length;
    Status status = UMC_OK;
    Ipp32u dirPart;    // prediction direction of current MB partition
    Ipp32s numParts;
    Ipp32s partCtr;
    Ipp8s RefIxL0 = 0;
    Ipp8s RefIxL1 = 0;
    Ipp8s *pRefIndexL0 = NULL;
    Ipp8s *pRefIndexL1 = NULL;

    H264DecoderMotionVector *pMVL0 = NULL;
    H264DecoderMotionVector *pMVL1 = NULL;

    Ipp32s uNumRefIdxL0Active;
    Ipp32s uNumRefIdxL1Active;
    Ipp32s uNumVLCCodes;

    switch (m_cur_mb.GlobalMacroblockInfo->mbtype)
    {
    case MBTYPE_FORWARD:
        num_vectorsL0 = 1;
        num_refIxL0 = 1;
        num_vectorsL1 = 0;
        num_refIxL1 = 0;
        dirPart = D_DIR_FWD;
        numParts = 1;
        break;
    case MBTYPE_BACKWARD:
        num_vectorsL0 = 0;
        num_refIxL0 = 0;
        num_vectorsL1 = 1;
        num_refIxL1 = 1;
        dirPart = D_DIR_BWD;
        numParts = 1;
        break;
    case MBTYPE_BIDIR:
        num_vectorsL0 = 1;
        num_refIxL0 = 1;
        num_vectorsL1 = 1;
        num_refIxL1 = 1;
        dirPart = D_DIR_BIDIR;
        numParts = 1;
        break;
    case MBTYPE_INTER_8x8:
    case MBTYPE_INTER_8x8_REF0:
        num_vectorsL0 = 0;
        num_refIxL0 = 0;
        num_vectorsL1 = 0;
        num_refIxL1 = 0;
        numParts = 0;
        for (block = 0; block<4; block++)
        {
            switch (m_cur_mb.GlobalMacroblockInfo->sbtype[block])
            {
            case SBTYPE_8x8:
                if ((m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_FWD) ||
                    (m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BIDIR))
                {
                    num_vectorsL0++;
                    num_refIxL0++;
                }
                if ((m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BWD) ||
                    (m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BIDIR))
                {
                    num_vectorsL1++;
                    num_refIxL1++;
                }
                numParts++;
                break;
            case SBTYPE_8x4:
            case SBTYPE_4x8:
                if ((m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_FWD) ||
                    (m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BIDIR))
                {
                    num_vectorsL0 += 2;
                    num_refIxL0++;        // only one per refIx per 8x8
                }
                if ((m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BWD) ||
                    (m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BIDIR))
                {
                    num_vectorsL1 += 2;
                    num_refIxL1++;
                }
                numParts += 2;
                break;
            case SBTYPE_4x4:
                if ((m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_FWD) ||
                    (m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BIDIR))
                {
                    num_vectorsL0 += 4;
                    num_refIxL0++;
                }
                if ((m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BWD) ||
                    (m_cur_mb.LocalMacroblockInfo->sbdir[block] == D_DIR_BIDIR))
                {
                    num_vectorsL1 += 4;
                    num_refIxL1++;
                }
                numParts += 4;
                break;
            case SBTYPE_DIRECT:
                numParts++;
                break;
            default:
                status = UMC_BAD_STREAM;
                goto done;
            }
        }
        dirPart = m_cur_mb.LocalMacroblockInfo->sbdir[0];
        break;
    case MBTYPE_INTER_16x8:
    case MBTYPE_INTER_8x16:
        num_vectorsL0 = 0;
        num_refIxL0 = 0;
        num_vectorsL1 = 0;
        num_refIxL1 = 0;
        if ((m_cur_mb.LocalMacroblockInfo->sbdir[0] == D_DIR_FWD) || (m_cur_mb.LocalMacroblockInfo->sbdir[0] == D_DIR_BIDIR))
        {
            num_vectorsL0++;
            num_refIxL0++;
        }
        if ((m_cur_mb.LocalMacroblockInfo->sbdir[0] == D_DIR_BWD) || (m_cur_mb.LocalMacroblockInfo->sbdir[0] == D_DIR_BIDIR))
        {
            num_vectorsL1++;
            num_refIxL1++;
        }
        if ((m_cur_mb.LocalMacroblockInfo->sbdir[1] == D_DIR_FWD) || (m_cur_mb.LocalMacroblockInfo->sbdir[1] == D_DIR_BIDIR))
        {
            num_vectorsL0++;
            num_refIxL0++;
        }
        if ((m_cur_mb.LocalMacroblockInfo->sbdir[1] == D_DIR_BWD) || (m_cur_mb.LocalMacroblockInfo->sbdir[1] == D_DIR_BIDIR))
        {
            num_vectorsL1++;
            num_refIxL1++;
        }
        numParts = 2;
        dirPart = m_cur_mb.LocalMacroblockInfo->sbdir[0];
        break;
    default:
        goto done;
    }

    // Get all of the reference index and MV VLC codes from the bitstream
    // The bitstream contains them in the following order, which is the
    // order they are returned in uVLCCodes:
    //   L0 reference index for all MB parts using L0 prediction
    //   L1 reference index for all MB parts using L1 prediction
    //   L0 MV delta for all MB parts using L0 prediction
    //   L1 MV delta for all MB parts using L1 prediction
    // Reference index data is present only when the number of active
    // reference frames is greater than one. Also, MBTYPE_INTER_8x8_REF0
    // type MB has no ref index info in the bitstream (use 0 for all).
    uNumRefIdxL0Active = m_pSliceHeader->num_ref_idx_l0_active<<pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    uNumRefIdxL1Active = m_pSliceHeader->num_ref_idx_l1_active<<pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    if (uNumRefIdxL0Active == 1 || MBTYPE_INTER_8x8_REF0 == m_cur_mb.GlobalMacroblockInfo->mbtype)
        num_refIxL0 = 0;
    if (uNumRefIdxL1Active == 1 || MBTYPE_INTER_8x8_REF0 == m_cur_mb.GlobalMacroblockInfo->mbtype)
        num_refIxL1 = 0;

    // set pointers into VLC codes array
    pVLCCodes = uVLCCodes;
    pVLCCodesL0RefIndex = uVLCCodes;
    pVLCCodesL1RefIndex = pVLCCodesL0RefIndex + num_refIxL0;
    pVLCCodesL0MV = pVLCCodesL1RefIndex + num_refIxL1;
    pVLCCodesL1MV = pVLCCodesL0MV + num_vectorsL0*2;

    uNumVLCCodes = (num_vectorsL0+num_vectorsL1)*2 + num_refIxL0 + num_refIxL1;

    // When possible ref index range is 0..1, the reference index is coded
    // as 1 bit. When that occurs, get the ref index codes separate from
    // the motion vectors. Otherwise get all of the codes at once.
    if (uNumRefIdxL0Active == 2 || uNumRefIdxL1Active == 2)
    {
        // 1 bit codes for at least one set of ref index codes
        if (uNumRefIdxL0Active == 2)
        {
            for (i=0; i<num_refIxL0; i++)
            {
                pVLCCodesL0RefIndex[i] = !m_pBitStream->Get1Bit();
            }
        }
        else if (uNumRefIdxL0Active > 2)
        {
            for (i=0;i<num_refIxL0;i++)
            {
                pVLCCodesL0RefIndex[i]=m_pBitStream->GetVLCElement_unsigned();
            }
        }

        if (uNumRefIdxL1Active == 2)
        {
            for (i=0; i<num_refIxL1; i++)
            {
                pVLCCodesL1RefIndex[i] = !m_pBitStream->Get1Bit();
            }
        }
        else if (uNumRefIdxL1Active > 2)
        {
            for (i=0;i<num_refIxL1;i++)
            {
                pVLCCodesL1RefIndex[i]=m_pBitStream->GetVLCElement_unsigned();
            }
        }

        // to get the MV for the MB
        uNumVLCCodes -= num_refIxL0 + num_refIxL1;
        pVLCCodes = pVLCCodesL0MV;
    }

    // get all MV and possibly Ref Index codes for the MB
    for (i=0;i<uNumVLCCodes;i++,pVLCCodes++)
    {
        *pVLCCodes=m_pBitStream->GetVLCElement_unsigned();
    }
    block = 0;
    subblock = 0;
    dec_vectorsL0 = 0;
    dec_vectorsL1 = 0;
    dec_refIxL0 = 0;
    dec_refIxL1 = 0;
    Ipp32u sboffset;
    for (partCtr = 0,sboffset=0; partCtr < numParts; partCtr++)
    {
        pMVL0 = &m_cur_mb.MVs[0]->MotionVectors[sboffset];
        pRefIndexL0 = &m_cur_mb.RefIdxs[0]->RefIdxs[sboffset];

        if (dirPart == D_DIR_FWD || dirPart == D_DIR_BIDIR)
        {    // L0
            if (num_refIxL0)
            {
                RefIxL0 = (Ipp8s)pVLCCodesL0RefIndex[dec_refIxL0];
                // dec_refIxL0 is incremented in mbtype-specific code below
                // instead of here because there is only one refIx for each
                // 8x8 block, so number of refIx values is not the same as
                // numParts.
                if (RefIxL0 >= (Ipp8s)uNumRefIdxL0Active || RefIxL0 < 0)
                {
                    status = UMC_BAD_STREAM;
                }
            }
            else
                RefIxL0 = 0;

            mvdx = (pVLCCodesL0MV[dec_vectorsL0*2]+1)>>1;
            if (!(pVLCCodesL0MV[dec_vectorsL0*2]&1))
                mvdx = -mvdx;

            mvdy = (pVLCCodesL0MV[dec_vectorsL0*2+1]+1)>>1;
            if (!(pVLCCodesL0MV[dec_vectorsL0*2+1]&1))
                mvdy = -mvdy;
            dec_vectorsL0++;

            // Find MV predictor
            ComputeMotionVectorPredictors(0, RefIxL0,
                                        block+subblock, &pmvx, &pmvy);

            mvxL0 = Ipp32s(mvdx + pmvx);
            mvyL0 = Ipp32s(mvdy + pmvy);

        }    // L0
        else
        {
            mvxL0 = mvyL0 = 0;
            RefIxL0 = -1;
        }
        if (bIsBSlice)
        {
            pMVL1 = &m_cur_mb.MVs[1]->MotionVectors[sboffset];
            pRefIndexL1 = &m_cur_mb.RefIdxs[1]->RefIdxs[sboffset];

            if (dirPart == D_DIR_BWD || dirPart == D_DIR_BIDIR)
            {
                if (num_refIxL1)
                {
                    RefIxL1 = (Ipp8s)pVLCCodesL1RefIndex[dec_refIxL1];
                    // dec_refIxL1 is incremented in mbtype-specific code below
                    // instead of here because there is only one refIx for each
                    // 8x8 block, so number of refIx values is not the same as
                    // numParts.
                    if (RefIxL1 >= (Ipp8s)uNumRefIdxL1Active || RefIxL1 < 0)
                    {
                        status = UMC_BAD_STREAM;
                    }
                }
                else
                    RefIxL1 = 0;

                mvdx = (Ipp16s)(pVLCCodesL1MV[dec_vectorsL1*2]+1)>>1;
                if (!(pVLCCodesL1MV[dec_vectorsL1*2]&1))
                    mvdx = -mvdx;

                mvdy = (Ipp16s)(pVLCCodesL1MV[dec_vectorsL1*2+1]+1)>>1;
                if (!(pVLCCodesL1MV[dec_vectorsL1*2+1]&1))
                    mvdy = -mvdy;
                dec_vectorsL1++;

                // Find MV predictor
                ComputeMotionVectorPredictors( 1, RefIxL1,block+subblock, &pmvx, &pmvy);

                mvxL1 = Ipp32s(mvdx + pmvx);
                mvyL1 = Ipp32s(mvdy + pmvy);

            }    // L1
            else
            {
                mvxL1 = mvyL1 = 0;
                RefIxL1 = -1;
            }
        }    // B slice

        // Store motion vectors and reference indexes into this frame's buffers
        switch (m_cur_mb.GlobalMacroblockInfo->mbtype)
        {
        case MBTYPE_FORWARD:
        case MBTYPE_BACKWARD:
        case MBTYPE_BIDIR:
            for (i = 0; i < 4; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    pMVL0[j].mvx = (Ipp16s) mvxL0;
                    pMVL0[j].mvy = (Ipp16s) mvyL0;
                    pRefIndexL0[j] = RefIxL0;
                }
                pMVL0 += 4;
                pRefIndexL0 += 4;
            }
            if (bIsBSlice)
            {
                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 4; j++)
                    {
                        pMVL1[j].mvx = (Ipp16s) mvxL1;
                        pMVL1[j].mvy = (Ipp16s) mvyL1;
                        pRefIndexL1[j] = RefIxL1;
                    }
                    pMVL1 += 4;
                    pRefIndexL1 += 4;
                }
            }
            break;
        case MBTYPE_INTER_16x8:
            // store in 8 top or bottom half blocks
            for (i = 0; i < 2; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    pMVL0[j].mvx = (Ipp16s) mvxL0;
                    pMVL0[j].mvy = (Ipp16s) mvyL0;
                    pRefIndexL0[j] = RefIxL0;
                }
                pMVL0 += 4;
                pRefIndexL0 += 4;
            }
            if (bIsBSlice)
            {
                for (i = 0; i < 2; i++)
                {
                    for (j = 0; j < 4; j++)
                    {
                        pMVL1[j].mvx = (Ipp16s) mvxL1;
                        pMVL1[j].mvy = (Ipp16s) mvyL1;
                        pRefIndexL1[j] = RefIxL1;
                    }
                    pMVL1 += 4;
                    pRefIndexL1 += 4;
                }
                if (dirPart == D_DIR_BWD || dirPart == D_DIR_BIDIR)
                    dec_refIxL1++;
            }
            block++;
            sboffset=8;
            if (dirPart == D_DIR_FWD || dirPart == D_DIR_BIDIR)
                dec_refIxL0++;
            dirPart = m_cur_mb.LocalMacroblockInfo->sbdir[block];        // next partition
            break;
        case MBTYPE_INTER_8x16:
            // store in 8 left or right half blocks
            for (i = 0; i < 4; i++)
            {
                for (j = 0; j < 2; j++)
                {
                    pMVL0[j].mvx = (Ipp16s) mvxL0;
                    pMVL0[j].mvy = (Ipp16s) mvyL0;
                    pRefIndexL0[j] = RefIxL0;
                }
                pMVL0 += 4;
                pRefIndexL0 += 4;
            }
            if (bIsBSlice)
            {
                // store in 8 left or right half blocks
                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 2; j++)
                    {
                        pMVL1[j].mvx = (Ipp16s) mvxL1;
                        pMVL1[j].mvy = (Ipp16s) mvyL1;
                        pRefIndexL1[j] = RefIxL1;
                    }
                    pMVL1 += 4;
                    pRefIndexL1 += 4;
                }
                if (dirPart == D_DIR_BWD || dirPart == D_DIR_BIDIR)
                    dec_refIxL1++;
            }
            block++;
            sboffset=2;
            if (dirPart == D_DIR_FWD || dirPart == D_DIR_BIDIR)
                dec_refIxL0++;
            dirPart = m_cur_mb.LocalMacroblockInfo->sbdir[block];        // next partition
            break;
        case MBTYPE_INTER_8x8:
        case MBTYPE_INTER_8x8_REF0:
            switch (m_cur_mb.GlobalMacroblockInfo->sbtype[block>>2])
            {
            case SBTYPE_8x8:
                pMVL0[0].mvx = (Ipp16s) mvxL0;
                pMVL0[0].mvy = (Ipp16s) mvyL0;
                pMVL0[1].mvx = (Ipp16s) mvxL0;
                pMVL0[1].mvy = (Ipp16s) mvyL0;
                pMVL0[4].mvx = (Ipp16s) mvxL0;
                pMVL0[4].mvy = (Ipp16s) mvyL0;
                pMVL0[4 + 1].mvx = (Ipp16s) mvxL0;
                pMVL0[4 + 1].mvy = (Ipp16s) mvyL0;
                pRefIndexL0[0] = RefIxL0;
                pRefIndexL0[1] = RefIxL0;
                pRefIndexL0[4] = RefIxL0;
                pRefIndexL0[4+1] = RefIxL0;


                if (bIsBSlice)
                {
                    pMVL1[0].mvx = (Ipp16s) mvxL1;
                    pMVL1[0].mvy = (Ipp16s) mvyL1;
                    pMVL1[1].mvx = (Ipp16s) mvxL1;
                    pMVL1[1].mvy = (Ipp16s) mvyL1;
                    pMVL1[4].mvx = (Ipp16s) mvxL1;
                    pMVL1[4].mvy = (Ipp16s) mvyL1;
                    pMVL1[4 + 1].mvx = (Ipp16s) mvxL1;
                    pMVL1[4 + 1].mvy = (Ipp16s) mvyL1;
                    pRefIndexL1[0] = RefIxL1;
                    pRefIndexL1[1] = RefIxL1;
                    pRefIndexL1[4] = RefIxL1;
                    pRefIndexL1[4+1] = RefIxL1;
                    if (dirPart == D_DIR_BWD || dirPart == D_DIR_BIDIR)
                        dec_refIxL1++;
                }
                if (block == 4)
                {
                    sboffset += 8 - 2;
                }
                else
                {
                    sboffset += 2;
                }
                block += 4;
                if (dirPart == D_DIR_FWD || dirPart == D_DIR_BIDIR)
                    dec_refIxL0++;
                break;
            case SBTYPE_8x4:
                pMVL0[0].mvx = (Ipp16s) mvxL0;
                pMVL0[0].mvy = (Ipp16s) mvyL0;
                pMVL0[1].mvx = (Ipp16s) mvxL0;
                pMVL0[1].mvy = (Ipp16s) mvyL0;
                pRefIndexL0[0] = RefIxL0;
                pRefIndexL0[1] = RefIxL0;
                if (bIsBSlice)
                {
                    pMVL1[0].mvx = (Ipp16s) mvxL1;
                    pMVL1[0].mvy = (Ipp16s) mvyL1;
                    pMVL1[1].mvx = (Ipp16s) mvxL1;
                    pMVL1[1].mvy = (Ipp16s) mvyL1;
                    pRefIndexL1[0] = RefIxL1;
                    pRefIndexL1[1] = RefIxL1;
                }
                if (subblock == 1)
                {
                    if (block == 4)
                    {
                        sboffset+=2;
                    }
                    else
                    {
                        sboffset-=2;
                    }
                    block += 4;
                    subblock = 0;
                    if (dirPart == D_DIR_FWD || dirPart == D_DIR_BIDIR)
                        dec_refIxL0++;
                    if (dirPart == D_DIR_BWD || dirPart == D_DIR_BIDIR)
                        dec_refIxL1++;
                }
                else
                {
                    subblock++;
                    sboffset  += 4;
                }
                break;
            case SBTYPE_4x8:
                pMVL0[0].mvx = (Ipp16s) mvxL0;
                pMVL0[0].mvy = (Ipp16s) mvyL0;
                pMVL0[4].mvx = (Ipp16s) mvxL0;
                pMVL0[4].mvy = (Ipp16s) mvyL0;
                pRefIndexL0[0] = RefIxL0;
                pRefIndexL0[4] = RefIxL0;
                if (bIsBSlice)
                {
                    pMVL1[0].mvx = (Ipp16s) mvxL1;
                    pMVL1[0].mvy = (Ipp16s) mvyL1;
                    pMVL1[4].mvx = (Ipp16s) mvxL1;
                    pMVL1[4].mvy = (Ipp16s) mvyL1;
                    pRefIndexL1[0] = RefIxL1;
                    pRefIndexL1[4] = RefIxL1;
                }
                if (subblock == 1)
                {
                    if (block == 4)
                    {
                        sboffset += 8 - 3;
                    }
                    else
                    {
                        sboffset++;
                    }
                    block += 4;
                    subblock = 0;
                    if (dirPart == D_DIR_FWD || dirPart == D_DIR_BIDIR)
                        dec_refIxL0++;
                    if (dirPart == D_DIR_BWD || dirPart == D_DIR_BIDIR)
                        dec_refIxL1++;
                }
                else
                {
                    subblock++;
                    sboffset++;
                }
                break;
            case SBTYPE_4x4:
                pMVL0[0].mvx = (Ipp16s) mvxL0;
                pMVL0[0].mvy = (Ipp16s) mvyL0;
                pRefIndexL0[0] = RefIxL0;
                if (bIsBSlice)
                {
                    pMVL1[0].mvx = (Ipp16s) mvxL1;
                    pMVL1[0].mvy = (Ipp16s) mvyL1;
                    pRefIndexL1[0] = RefIxL1;
                }
                sboffset += (xyoff[block+subblock][0]>>2) +
                    ((xyoff[block+subblock][1])>>2)*4;
                if (subblock == 3)
                {
                    block += 4;
                    subblock = 0;
                    if (dirPart == D_DIR_FWD || dirPart == D_DIR_BIDIR)
                        dec_refIxL0++;
                    if (dirPart == D_DIR_BWD || dirPart == D_DIR_BIDIR)
                        dec_refIxL1++;
                }
                else
                {
                    subblock++;
                }
                break;
            case SBTYPE_DIRECT:
                // nothing to do except advance to next 8x8 partition
                if (block == 4)
                {
                    sboffset += 8 - 2;
                }
                else
                {
                    sboffset += 2;
                }
                block += 4;
                break;
            }    // 8x8 switch sbtype
            if (block<16)
            dirPart = m_cur_mb.LocalMacroblockInfo->sbdir[block>>2];        // next partition
            break;
        }    // switch mbtype
    }    // for partCtr

done:
    return status;

} // Status H264SegmentDecoder::DecodeMotionVectors(bool bIsBSlice)

bool H264SegmentDecoder::DecodeSkipMotionVectors()
{
    Ipp32s pmvx, pmvy;
    Ipp32s mvx = 0, mvy = 0;
    Ipp32s px0, px1;
    Ipp32s py0, py1;
    Ipp8s slRefIx;    // ref index, sb to the left
    Ipp8s saRefIx;    // ref index, sb to the left

    bool bUseZeroMV = true;
    bool bAboveMVIsZero;
    bool bLeftMVIsZero;
    H264DecoderMotionVector *sl; // pointer to MV, sb to the left
    H264DecoderMotionVector *sa; // pointer to MV, sb above
    H264DecoderMotionVector *MV = m_cur_mb.MVs[0]->MotionVectors;
    Ipp8s *RefIndex = m_cur_mb.RefIdxs[0]->RefIdxs;
    ///////////////////////////////////////////////////////////////////
    if (m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num>=0 && m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num>=0)
    {
        // Both above and left are available
        sl = &m_pCurrentFrame->m_mbinfo.MV[0][m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].MotionVectors[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].block_num];
        sa = &m_pCurrentFrame->m_mbinfo.MV[0][m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].MotionVectors[m_cur_mb.CurrentBlockNeighbours.mb_above.block_num];
        slRefIx = m_pCurrentFrame->m_mbinfo.RefIdxs[0][m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].block_num];
        saRefIx = m_pCurrentFrame->m_mbinfo.RefIdxs[0][m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mb_above.block_num];

        px0 = sl->mvx;
        py0 = sl->mvy;
        px1 = sa->mvx;
        py1 = sa->mvy;
        if (pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo) &&
            !GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num]))
        {
            py0 /= 2;
            slRefIx *=2;
        }
        if (!pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo) &&
            GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num]))
        {
            py0 *= 2;
            slRefIx  >>=1;
        }
        if (pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)  &&
            !GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num]))
        {
            py1 /= 2;
            saRefIx*=2;
        }
        if (!pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)  &&
            GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num]))
        {
            py1 *= 2;
            saRefIx>>=1;
        }


        // Use prediction if both above and left MV are not zero
        //        Ipp32u mb_width = m_pCurrentFrame->macroBlockSize().width;
        //        mbtypeAbove = TopMB->mbtype;
        //        mbtypeLeft = LeftMB->mbtype;

        bAboveMVIsZero = px1==0 && py1==0 && saRefIx==0;
        bLeftMVIsZero = px0==0 && py0==0 && slRefIx==0;
        if (!bAboveMVIsZero && !bLeftMVIsZero)
        {
            // Find MV predictor
            ComputeMotionVectorPredictors(0, 0, 0, &pmvx, &pmvy);
            mvx = pmvx;
            mvy = pmvy;
            bUseZeroMV = false;
        }
        else
        {
#ifdef STORE_PREDICTORS
            if (mvs==NULL) mvs=fopen(__PREDICTORS_FILE__,"w+t");
            if (mvs)
            {
                char slice[]={'P','B','I'};
                fprintf(mvs,"%d 0,0 %d %c\n",numvecs++,MBAddr2MBCount(m_CurMBAddr),slice[m_pSliceHeader->slice_type]);
            }
#endif

        }
    }
    else
    {
#ifdef STORE_PREDICTORS
        if (mvs==NULL) mvs=fopen(__PREDICTORS_FILE__,"w+t");
        if (mvs)
        {
            char slice[]={'P','B','I'};
            fprintf(mvs,"%d 0,0 %d %c\n",numvecs++,MBAddr2MBCount(m_CurMBAddr),slice[m_pSliceHeader->slice_type]);
        }
#endif
    }

    if (!bUseZeroMV)
    {
        // Store motion vectors in Sub Block Data buffer
        int i, j;
        for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 4; j++)
            {
                MV[j].mvx = (Ipp16s) mvx;
                MV[j].mvy = (Ipp16s) mvy;
                RefIndex[j] = 0;
            }
            MV += 4;
            RefIndex += 4;
        }
    }

    return bUseZeroMV;

} // bool H264SegmentDecoder::DecodeSkipMotionVectors()

///////////////////////////////////////////////////////////////////////////////
// decodePCMCoefficients
//
// Extracts raw coefficients from bitstream by:
//  a) byte aligning bitstream pointer
//  b) copying bitstream pointer to m_pCoeffBlocksWrite
//  c) advancing bitstream pointer by 256+128 bytes
//
//    Also initializes NumCoef buffers for correct use in future MBs.
//
/////////////////////////////
const Ipp32u num_coeffs[4]={256,384,512,768};

Status H264SegmentDecoder::DecodeCoefficients_PCM(Ipp8u color_format)
{
    Status ps = UMC_OK;

    Ipp32u length = num_coeffs[color_format];
    // number of raw coeff bits
    // to write pointer to non-aligned m_pCoeffBlocksWrite
    m_cur_mb.LocalMacroblockInfo->QP=0;
    Ipp8u *pCoeffBlocksWrite = reinterpret_cast<Ipp8u *> (m_pCoeffBlocksWrite);

#ifndef DROP_CABAC
	if (m_pPicParamSet->entropy_coding_mode)
    {
        m_pBitStream->TerminateDecode_CABAC();
    }
    else
#else
	if( m_pPicParamSet->entropy_coding_mode==1 )
		return UMC_UNSUPPORTED;
#endif
	{
        m_pBitStream->AlignPointerRight();
    }

    for (Ipp32u i = 0; i<length; i++)
    {
        pCoeffBlocksWrite[i] = (Ipp8u) m_pBitStream->GetBits(8);
    }

    memset(m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff,16,48);//set correct numcoeffs

#ifndef DROP_CABAC
    if(m_pPicParamSet->entropy_coding_mode)
    {
        m_pBitStream->InitializeDecodingEngine_CABAC();
/*
        if(m_pSliceHeader->slice_type == INTRASLICE)
        {
            m_pBitStream->InitializeContextVariablesIntra_CABAC(
                m_pPicParamSet->pic_init_qp +
                m_pSliceHeader->slice_qp_delta);
        }
        else
        {
            m_pBitStream->InitializeContextVariablesInter_CABAC(
                m_pPicParamSet->pic_init_qp +
                m_pSliceHeader->slice_qp_delta, m_pSliceHeader->cabac_init_idc);
        }*/
    }
#else
	if( m_pPicParamSet->entropy_coding_mode==1 )
		return UMC_UNSUPPORTED;
#endif

    m_pCoeffBlocksWrite = (Ipp16s *) (pCoeffBlocksWrite + length);

    return ps;

} // Status H264SegmentDecoder::DecodeCoefficients_PCM(Ipp8u color_format)

// Used to obtain colocated motion vector and reference index for temporal
// direct B. Called only when the colocated MB is not INTRA. Uses the
// L0 and L1 reference indices of colocated MB to choose MV. Also translates
// the colocated reference index to be used into the correct L0 index
// for this slice.
void H264SegmentDecoder::GetDirectTemporalMV(Ipp32s MBCol,
                                             Ipp32u ipos, // offset into MV and RefIndex storage
                                             H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                             H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                             H264DecoderMotionVector  &MVL0, // return colocated MV here
                                             Ipp8s &RefIndexL0) // return ref index here
{
    //I'm not sure about this function correctness
    VM_ASSERT(pRefPicList1[0]);
    H264DecoderMotionVector   *pRefMVL0;
    H264DecoderMotionVector   *pRefMVL1;
    Ipp8s *pRefRefIndexL0;
    Ipp8s *pRefRefIndexL1;
    H264VideoDecoder::H264DecoderFrame **pRefRefPicList;
    Ipp32s RefPicNum;
    bool bFound = false;
    // Set pointers to colocated list 0 ref index and MV
    pRefRefIndexL0 = &pRefPicList1[0]->m_mbinfo.RefIdxs[0][MBCol].RefIdxs[ipos];
    pRefMVL0 = &pRefPicList1[0]->m_mbinfo.MV[0][MBCol].MotionVectors[ipos];
    Ipp16u uRefSliceNum=pRefPicList1[0]->m_mbinfo.mbs[MBCol].slice_id;

    VM_ASSERT(pRefRefIndexL0);
    VM_ASSERT(pRefMVL0);

    // Get ref index and MV from L0 of colocated ref MB if the
    // colocated L0 ref index is >=0. Else use L1.
    RefIndexL0 = *pRefRefIndexL0;
    if (RefIndexL0 >= 0)
    {
        // Use colocated L0
        MVL0 = *pRefMVL0;

        // Get pointer to ref pic list 0 of colocated
        pRefRefPicList = pRefPicList1[0]->GetRefPicListSafe(uRefSliceNum, 0);
        //***pRefRefPicList = pRefPicList1[0]->GetRefPicList(uRefSliceNum, 0)->m_RefPicList;
    }
    else
    {
        // Use Ref L1

        // Set pointers to colocated list 1 ref index and MV
        pRefRefIndexL1 = &pRefPicList1[0]->m_mbinfo.RefIdxs[1][MBCol].RefIdxs[ipos];
        pRefMVL1 = &pRefPicList1[0]->m_mbinfo.MV[1][MBCol].MotionVectors[ipos];
        VM_ASSERT(pRefRefIndexL1);
        VM_ASSERT(pRefMVL1);
        RefIndexL0 = *pRefRefIndexL1;

        // Use colocated L1
        MVL0 = *pRefMVL1;

        // Get pointer to ref pic list 1 of colocated
        pRefRefPicList = pRefPicList1[0]->GetRefPicListSafe(uRefSliceNum, 1);
        //***pRefRefPicList = pRefPicList1[0]->GetRefPicList(uRefSliceNum, 1)->m_RefPicList;

    }

    VM_ASSERT(pRefRefPicList[RefIndexL0]);

    // Translate the reference index of the colocated to current
    // L0 index to the same reference picture, using PicNum or
    // LongTermPicNum as id criteria.
    if (pRefRefPicList[RefIndexL0]->isShortTermRef())
    {
        RefPicNum = pRefRefPicList[RefIndexL0]->PicNum(0,3);

        // find matching reference frame on current slice list 0
        RefIndexL0 = 0;
        while (!bFound)
        {
            if (pRefPicList0[RefIndexL0] == NULL)
                break;  // reached end of valid entries without a match
            if (pRefPicList0[RefIndexL0]->isShortTermRef() &&
                pRefPicList0[RefIndexL0]->PicNum(0,3) == RefPicNum)
                bFound = true;
            else
                RefIndexL0++;
        }
        if (!bFound)
        {
            // can't happen
            VM_ASSERT(bFound);
            RefIndexL0 = 0;
        }
    }
    else if (pRefRefPicList[RefIndexL0]->isLongTermRef())
    {
        RefPicNum = pRefRefPicList[RefIndexL0]->LongTermPicNum(0,3);

        // find matching reference frame on current slice list 0
        RefIndexL0 = 0;
        while (!bFound)
        {
            if (pRefPicList0[RefIndexL0] == NULL)
                break;  // reached end of valid entries without a match
            if (pRefPicList0[RefIndexL0]->isLongTermRef() &&
                pRefPicList0[RefIndexL0]->LongTermPicNum(0,3) == RefPicNum)
                bFound = true;
            else
                RefIndexL0++;
        }
        if (!bFound)
        {
            // can't happen
            VM_ASSERT(bFound);
            RefIndexL0 = 0;
        }
    }
    else
    {
        // colocated is in a reference that is not marked as short-term
        // or long-term, should not happen
        // Well it can be happen since in case of num_ref_frames=1 and this frame is already unmarked as reference
        RefIndexL0 = 0;     // correct value
    }
    VM_ASSERT(RefIndexL0 >= 0);

} // void H264SegmentDecoder::GetDirectTemporalMV(Ipp32s MBCol,


void H264SegmentDecoder::DecodeDirectMotionVectorsTemporal(Ipp32u sboffset, // offset into MV and RefIndex storage
                                                           H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                                           H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                                           Ipp8s *pFields0,
                                                           Ipp8s *pFields1,
                                                           bool bIs8x8)
{
    // Use forward and backward ratios to scale the reference vectors to
    // produce the forward and backward vectors, for either a 16x16 macroblock
    // (16 vectors) or an 8x8 block (4 vectors)

    // When bRefMBIsInter is false, set the MV to zero; for that case,
    // decodeDirectMotionVectors_8x8Inference is used because it is faster.

    Ipp32s *pDistScaleFactorMV;
    Ipp32u NumVecOneDim = bIs8x8 ? 2 : 4;
    Ipp32u xpos, ypos;
    //    Ipp32u roi.width = m_pCurrentFrame->subBlockSize().width;
    Ipp8s RefIndexL0 = 0, RefIndexL1 = 0;
    H264DecoderMotionVector MV = {0};
    int mvxf, mvyf, mvxb, mvyb;

    // set up pointers to where MV and RefIndex will be stored
    H264DecoderMotionVector *pFwdMV = &m_cur_mb.MVs[0]->MotionVectors[sboffset];
    H264DecoderMotionVector *pBwdMV = &m_cur_mb.MVs[1]->MotionVectors[sboffset];
	
    //***bnie Ipp32u scale_idx = pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);//***bnie: | (m_pCurrentFrame->m_PictureStructureForDec<FRM_STRUCTURE);
    Ipp8s *pRefIndexL0 = &m_cur_mb.RefIdxs[0]->RefIdxs[sboffset];
    Ipp8s *pRefIndexL1 = &m_cur_mb.RefIdxs[1]->RefIdxs[sboffset];
    RefIndexL1 = 0;
    for (ypos=0; ypos<NumVecOneDim; ypos++)
    {
        Ipp32s ref_mvoffset=ypos*4+sboffset;
        Ipp8s scale;
        Ipp32s MBCol;
        MBCol=GetColocatedLocation(pRefPicList1[0],pFields1[0],ref_mvoffset,&scale);
        bool bRefMBIsInter = IS_INTER_MBTYPE(pRefPicList1[0]->m_mbinfo.mbs[MBCol].mbtype);
        for (xpos=0; xpos<NumVecOneDim; xpos++)
        {
            if (bRefMBIsInter)
            {
                // Get colocated MV and translated ref index
                //***bnie: switch(m_pCurrentFrame->m_PictureStructureForDec)
                //***bnie: {
                //***bnie: case FRM_STRUCTURE:
                    GetDirectTemporalMV(MBCol,xpos+ref_mvoffset,
                        pRefPicList0, pRefPicList1,MV,RefIndexL0);
                //***bnie:     break;
                //***bnie: }

                if (pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo))
                {
                    Ipp8s curfield = pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
                    Ipp8s ref1field = curfield;
                    Ipp8s ref0field = curfield ^ (RefIndexL0&1);
                    pDistScaleFactorMV = m_pSlice->GetDistScaleFactorMVAFF()[curfield][ref1field][ref0field];
                }
                else
                {
                    pDistScaleFactorMV = m_pSlice->GetDistScaleFactorMV();
                }
                switch(scale)
                {
                case 1: MV.mvy/=2;
                    break;
                case -1:MV.mvy*=2;
                    break;
                }
                mvxf = int
                    ((MV.mvx * pDistScaleFactorMV[RefIndexL0] + 128) >> 8);
                mvxb = mvxf - MV.mvx;
                mvyf = int
                    ((MV.mvy * pDistScaleFactorMV[RefIndexL0] + 128) >> 8);
                mvyb = mvyf - MV.mvy;
                pFwdMV[xpos].mvx = (Ipp16s) mvxf;
                pFwdMV[xpos].mvy = (Ipp16s) mvyf;
                pBwdMV[xpos].mvx = (Ipp16s) mvxb;
                pBwdMV[xpos].mvy = (Ipp16s) mvyb;
            }
            else
            {
                pFwdMV[xpos].mvx = 0;
                pFwdMV[xpos].mvy = 0;
                pBwdMV[xpos].mvx = 0;
                pBwdMV[xpos].mvy = 0;
                RefIndexL0 = 0;
                RefIndexL1 = 0;
            }
            pRefIndexL0[xpos] = RefIndexL0;
            pRefIndexL1[xpos] = RefIndexL1;
            m_cur_mb.LocalMacroblockInfo->sbdir[subblock_block_membership[ypos*4+xpos+sboffset]]=D_DIR_DIRECT;
            m_cur_mb.GlobalMacroblockInfo->sbtype[subblock_block_membership[ypos*4+xpos+sboffset]]=SBTYPE_DIRECT;

        }   // for xpos

        pFwdMV += 4;
        pBwdMV += 4;
        pRefIndexL0 += 4;
        pRefIndexL1 += 4;

    }   // for ypos

} // void H264SegmentDecoder::DecodeDirectMotionVectorsTemporal(Ipp32u sboffset,

void H264SegmentDecoder::DecodeDirectMotionVectorsTemporal_8x8Inference(H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                                                        H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                                                        Ipp8s *pFields0,
                                                                        Ipp8s *pFields1,
                                                                        Ipp32s iWhich8x8) // 0..3 if 8x8 block; -1 if 16x16
{
    // Use forward and backward ratios to scale the reference vectors to
    // produce the forward and backward vectors, for either a 16x16 macroblock
    // (16 vectors) or an 8x8 block (4 vectors)

    // When bRefMBIsInter is false, set the MV to zero.

    Ipp32s *pDistScaleFactorMV;
    Ipp32u sb;
    Ipp32s ref_mvoffset, sboffset = 0;
    //Ipp32u roi.width = m_pCurrentFrame->subBlockSize().width;
    Ipp8s RefIndexL0 = 0, RefIndexL1 = 0;
    H264DecoderMotionVector  MV = {0};
    int mvxf, mvyf, mvxb, mvyb;

    // set up pointers to where MV and RefIndex will be stored
    H264DecoderMotionVector *pFwdMV = m_cur_mb.MVs[0]->MotionVectors;
    H264DecoderMotionVector *pBwdMV = m_cur_mb.MVs[1]->MotionVectors;
    Ipp8s *pRefIndexL0 = m_cur_mb.RefIdxs[0]->RefIdxs;
    Ipp8s *pRefIndexL1 = m_cur_mb.RefIdxs[1]->RefIdxs;
    //***bnie Ipp32u scale_idx = pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    sb = iWhich8x8 == -1 ? 0 : iWhich8x8;
    for (; sb<4; sb++)
    {
        switch (sb)
        {
        case 0:
            ref_mvoffset = 0;   // upper left corner
            sboffset = 0;
            break;
        case 1:
            ref_mvoffset = 3;   // upper right corner
            sboffset = 2;
            break;
        case 2:
            ref_mvoffset = 12;   // lower left corner
            sboffset = 8;
            break;
        case 3:
            ref_mvoffset = 3 + 12;   // lower right corner
            sboffset = 2 + 8;
            break;
        }
        Ipp32s MBCol;
        Ipp8s scale;
        MBCol=GetColocatedLocation(pRefPicList1[0],pFields1[0],ref_mvoffset,&scale);
        bool bRefMBIsInter = IS_INTER_MBTYPE(pRefPicList1[0]->m_mbinfo.mbs[MBCol].mbtype);
        RefIndexL1 = 0;
        if (bRefMBIsInter)
        {
            // Get colocated MV and translated ref index
            //***bnie: switch(m_pCurrentFrame->m_PictureStructureForDec)
            //***bnie: {
            //***bnie: case FRM_STRUCTURE:
                GetDirectTemporalMV(MBCol,ref_mvoffset,
                    pRefPicList0, pRefPicList1,MV,RefIndexL0);
            //***bnie:     break;
			//***bnie: }

            {
                pDistScaleFactorMV = m_pSlice->GetDistScaleFactorMV();
            }

            switch(scale)
            {
            case 1: MV.mvy/=2;
                break;
            case -1:MV.mvy*=2;
                break;
            }
            // Reference MV from outside corner 4x4
            mvxf = int
                ((MV.mvx * pDistScaleFactorMV[RefIndexL0] + 128) >> 8);
            mvxb = mvxf - MV.mvx;
            mvyf = int
                ((MV.mvy * pDistScaleFactorMV[RefIndexL0] + 128) >> 8);
            mvyb = mvyf - MV.mvy;
        }
        else
        {
            mvxf = 0;
            mvyf = 0;
            mvxb = 0;
            mvyb = 0;
            RefIndexL0 = 0;
        }

        // Save MV to all 4 4x4's for this 8x8.
        pFwdMV[sboffset].mvx = (Ipp16s) mvxf;
        pFwdMV[sboffset].mvy = (Ipp16s) mvyf;
        pBwdMV[sboffset].mvx = (Ipp16s) mvxb;
        pBwdMV[sboffset].mvy = (Ipp16s) mvyb;

        pFwdMV[sboffset+1].mvx = (Ipp16s) mvxf;
        pFwdMV[sboffset+1].mvy = (Ipp16s) mvyf;
        pBwdMV[sboffset+1].mvx = (Ipp16s) mvxb;
        pBwdMV[sboffset+1].mvy = (Ipp16s) mvyb;

        pFwdMV[sboffset+4].mvx = (Ipp16s) mvxf;
        pFwdMV[sboffset+4].mvy = (Ipp16s) mvyf;
        pBwdMV[sboffset+4].mvx = (Ipp16s) mvxb;
        pBwdMV[sboffset+4].mvy = (Ipp16s) mvyb;

        pFwdMV[sboffset+4+1].mvx = (Ipp16s) mvxf;
        pFwdMV[sboffset+4+1].mvy = (Ipp16s) mvyf;
        pBwdMV[sboffset+4+1].mvx = (Ipp16s) mvxb;
        pBwdMV[sboffset+4+1].mvy = (Ipp16s) mvyb;

        pRefIndexL0[sboffset] = RefIndexL0;
        pRefIndexL1[sboffset] = RefIndexL1;

        pRefIndexL0[sboffset+1] = RefIndexL0;
        pRefIndexL1[sboffset+1] = RefIndexL1;

        pRefIndexL0[sboffset+4] = RefIndexL0;
        pRefIndexL1[sboffset+4] = RefIndexL1;

        pRefIndexL0[sboffset+4+1] = RefIndexL0;
        pRefIndexL1[sboffset+4+1] = RefIndexL1;
        m_cur_mb.LocalMacroblockInfo->sbdir[sb]=D_DIR_DIRECT;
        m_cur_mb.GlobalMacroblockInfo->sbtype[sb]=SBTYPE_DIRECT;
        // Quit if block is 8x8
        if (iWhich8x8 != -1)
            break;

    }   // for sb

} // void H264SegmentDecoder::DecodeDirectMotionVectorsTemporal_8x8Inference(H264DecoderFrame **pRefPicList0,

void H264SegmentDecoder::ComputeDirectSpatialRefIdx(Ipp8s *pRefIndexL0, // neighbor RefIdx relative to this, forward result here
                                                    Ipp8s *pRefIndexL1) // neighbor RefIdx relative to this, backward result here
{
    const Ipp8s *pRefIxlL0, *pRefIxlL1;
    const Ipp8s *pRefIxaL0, *pRefIxaL1;
    const Ipp8s *pRefIxrL0, *pRefIxrL1;

    Ipp32u uRefIxlL0, uRefIxlL1;
    Ipp32u uRefIxaL0, uRefIxaL1;
    Ipp32u uRefIxrL0, uRefIxrL1;

    Ipp8s nullRefIx = -1;
    Ipp32u uRefIxL0, uRefIxL1;
    //    Ipp32u mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32u lbls=0,lbrs=0,tbls=0,tbrs=0,rbls=0,rbrs=0;
    H264DecoderMacroblockGlobalInfo *gmbs;
    gmbs = m_pCurrentFrame->m_mbinfo.mbs;

    if (m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num>=0)
    {
        lbls=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num]))>0;
        lbrs=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num]))<0;
        pRefIxlL0=&m_pCurrentFrame->m_mbinfo.RefIdxs[0][m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].block_num];
        pRefIxlL1=&m_pCurrentFrame->m_mbinfo.RefIdxs[1][m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].block_num];
    }
    else
    {
        // correct for left edge
        pRefIxlL0 = &nullRefIx;
        pRefIxlL1 = &nullRefIx;
    }

    if (m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num>=0)
    {
        tbls=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num]))>0;
        tbrs=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num]))<0;
        pRefIxaL0=&m_pCurrentFrame->m_mbinfo.RefIdxs[0][m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mb_above.block_num];
        pRefIxaL1=&m_pCurrentFrame->m_mbinfo.RefIdxs[1][m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mb_above.block_num];
    }
    else
    {
        // correct for top edge
        pRefIxaL0 = &nullRefIx;
        pRefIxaL1 = &nullRefIx;
    }

    // correct for upper right
    if (m_cur_mb.CurrentBlockNeighbours.mb_above_right.mb_num>=0)
    {
        rbls=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mb_above_right.mb_num]))>0;
        rbrs=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mb_above_right.mb_num]))<0;
        pRefIxrL0=&m_pCurrentFrame->m_mbinfo.RefIdxs[0][m_cur_mb.CurrentBlockNeighbours.mb_above_right.mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mb_above_right.block_num];
        pRefIxrL1=&m_pCurrentFrame->m_mbinfo.RefIdxs[1][m_cur_mb.CurrentBlockNeighbours.mb_above_right.mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mb_above_right.block_num];
    }
    else if (m_cur_mb.CurrentBlockNeighbours.mb_above_left.mb_num>=0)
    {
        rbls=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mb_above_left.mb_num]))>0;
        rbrs=(pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo)-GetMBFieldDecodingFlag(gmbs[m_cur_mb.CurrentBlockNeighbours.mb_above_left.mb_num]))<0;
        pRefIxrL0=&m_pCurrentFrame->m_mbinfo.RefIdxs[0][m_cur_mb.CurrentBlockNeighbours.mb_above_left.mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mb_above_left.block_num];
        pRefIxrL1=&m_pCurrentFrame->m_mbinfo.RefIdxs[1][m_cur_mb.CurrentBlockNeighbours.mb_above_left.mb_num].RefIdxs[m_cur_mb.CurrentBlockNeighbours.mb_above_left.block_num];
    }
    else
    {
        pRefIxrL0 = &nullRefIx;
        pRefIxrL1 = &nullRefIx;
    }

    // Returned index is positiveMIN(a,b,c), -1 is returned only when all are <0.
    // Smallest refIdx is -1, convert to unsigned for make positiveMIN faster.
    uRefIxlL0 = (Ipp32u)(*pRefIxlL0 & 0x7f);
    uRefIxlL1 = (Ipp32u)(*pRefIxlL1 & 0x7f);
    uRefIxaL0 = (Ipp32u)(*pRefIxaL0 & 0x7f);
    uRefIxaL1 = (Ipp32u)(*pRefIxaL1 & 0x7f);
    uRefIxrL0 = (Ipp32u)(*pRefIxrL0 & 0x7f);
    uRefIxrL1 = (Ipp32u)(*pRefIxrL1 & 0x7f);

    uRefIxL0 = MIN(((uRefIxlL0<<lbls)>>lbrs), MIN(((uRefIxaL0<<tbls)>>tbrs), ((uRefIxrL0<<rbls)>>rbrs)));
    uRefIxL1 = MIN(((uRefIxlL1<<lbls)>>lbrs), MIN(((uRefIxaL1<<tbls)>>tbrs), ((uRefIxrL1<<rbls)>>rbrs)));
    *pRefIndexL0 = uRefIxL0 >= 0x3f ? -1 : (Ipp8s)uRefIxL0;
    *pRefIndexL1 = uRefIxL1 >= 0x3f ? -1 : (Ipp8s)uRefIxL1;

} // void H264SegmentDecoder::ComputeDirectSpatialRefIdx(Ipp8s *pRefIndexL0,

Status H264SegmentDecoder::DecodeDirectMotionVectorsSpatial(H264VideoDecoder::H264DecoderFrame ** /*pRefPicList0*/,
                                                            H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                                            Ipp8u Field,
                                                            bool bUseDirect8x8Inference)
{
    Status ps=UMC_OK;
    Ipp32u xpos, ypos;
    //    Ipp32u roi.width = m_pCurrentFrame->subBlockSize().width;
    Ipp32s mvxL0, mvyL0;
    Ipp32s mvxL1, mvyL1;
    Ipp8s RefIndexL0, RefIndexL1;
/*    bool bMaybeUseZeroPred;*/
    bool bUseZeroPredL0, bUseZeroPredL1;
    bool bBothRefIdxNoExist;
    bool bL1RefPicisShortTerm;
    Ipp32u uSaveMBType = m_cur_mb.GlobalMacroblockInfo->mbtype;
    Ipp32u uPredDir;    // prediction direction of macroblock
    VM_ASSERT(pRefPicList1[0]);
    //Ipp32s        mbcol = getColocatedMB(pMBInfo,pRefPicList1,yM);
    //    const PackedMV *pRefMVL0 = &pRefPicList1[0]->m_pMBInfo[mbcol].MVS[0][yM];
    //    const PackedMV *pRefMVL1 = &pRefPicList1[0]->m_pMBInfo[mbcol].MVS[1][yM];
    //    const Ipp8s *pColocatedRefIndexL0 = &pRefPicList1[0]->m_pMBInfo[mbcol].refIdxs[0][yM];
    //    const Ipp8s *pColocatedRefIndexL1 = &pRefPicList1[0]->m_pMBInfo[mbcol].refIdxs[1][yM];
    bL1RefPicisShortTerm  = pRefPicList1[0]->isShortTermRef_kinoma_always_frame();//***bnie: (Ipp8s) m_field_index_kinoma_always_zero);
    //  bool bRefMBIsInter = IS_INTER_MBTYPE(pRefPicList1[0]->m_pMBInfo[mbcol].mbtype);
    //    Ipp8s RefIndex = bRefMBIsInter ? 0: -1;

    // set up pointers to where MV and RefIndex will be stored
    H264DecoderMacroblockMVs *pFwdMV = m_cur_mb.MVs[0];
    H264DecoderMacroblockMVs *pBwdMV = m_cur_mb.MVs[1];
    Ipp8s *pFwdRefIndex = m_cur_mb.RefIdxs[0]->RefIdxs;
    Ipp8s *pBwdRefIndex = m_cur_mb.RefIdxs[1]->RefIdxs;
    bool bAll4x4AreSame = true;
    bool bAll8x8AreSame = true;
    // Because predicted MV is computed using 16x16 block it is likely
    // that all 4x4 blocks will use the same MV and reference frame.
    // It is possible, however, for the MV for any 4x4 block to be set
    // to 0,0 instead of the computed MV. This possibility by default
    // forces motion compensation to be performed for each 4x4, the slowest
    // possible option. These booleans are used to detect when all of the
    // 4x4 blocks in an 8x8 can be combined for motion comp, and even better,
    // when all of the 8x8 blocks in the macroblock can be combined.

    // Change mbtype to any INTER 16x16 type, for computeMV function,
    // required for the 8x8 DIRECT case to force computeMV to get MV
    // using 16x16 type instead.
    m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_FORWARD;
    // Find the ref idx values to use.
    ComputeDirectSpatialRefIdx(pFwdRefIndex, pBwdRefIndex);

    // Copy to local vars to avoid lots of pointer derefs
    RefIndexL0 = *pFwdRefIndex;
    RefIndexL1 = *pBwdRefIndex;
    // Forward MV (L0)
    if (RefIndexL0 != -1)
    {
        // L0 ref idx exists, use to obtain predicted L0 motion vector
        // for the macroblock
        ComputeMotionVectorPredictors(0,
            RefIndexL0,
            0,  // block
            &mvxL0, &mvyL0);
    }
    else
    {
        mvxL0 = mvyL0 = 0;

    }
    // Backward MV (L1)
    if (RefIndexL1 != -1)
    {
        // L0 ref idx exists, use to obtain predicted L0 motion vector
        // for the macroblock
        ComputeMotionVectorPredictors(1,
            RefIndexL1,
            0,  // block
            &mvxL1, &mvyL1);
    }
    else
    {
        mvxL1 = mvyL1 = 0;
    }
    // restore mbtype
    m_cur_mb.GlobalMacroblockInfo->mbtype = (Ipp8u)uSaveMBType;
    // set direction for MB
    if ( (RefIndexL0 != -1) && (RefIndexL1 == -1) )
        uPredDir = D_DIR_DIRECT_SPATIAL_FWD;
    else if ( (RefIndexL0 == -1) && (RefIndexL1 != -1) )
        uPredDir = D_DIR_DIRECT_SPATIAL_BWD;
    else
        uPredDir = D_DIR_DIRECT_SPATIAL_BIDIR;

    // In loops below, set MV and RefIdx for all subblocks. Conditionally
    // change MV to 0,0 and RefIndex to 0 (doing so called UseZeroPred here).
    // To select UseZeroPred for a part:
    //  (RefIndexLx < 0) ||
    //  (bL1RefPicisShortTerm && RefIndexLx==0 && ColocatedRefIndex==0 &&
    //    (colocated motion vectors in range -1..+1)
    // When both RefIndexLx are -1, ZeroPred is used and both RefIndexLx
    // are changed to zero.

    // It is desirable to avoid checking the colocated motion vectors and
    // colocated ref index, bools and orders of conditional testing are
    // set up to do so.

    // At the MB level, the colocated do not need to be checked if:
    //  - both RefIndexLx < 0
    //  - colocated is INTRA (know all colocated RefIndex are -1)
    //  - L1 Ref Pic is not short term
    // set bMaybeUseZeroPred to true if any of the above are false

    bBothRefIdxNoExist = (RefIndexL0 == -1) && (RefIndexL1 == -1);

    // Set MV for all DIRECT 4x4 subblocks
    Ipp32s sb, sbcolpartoffset, sbpartoffset;
    Ipp8s colocatedRefIndex;
    H264DecoderMotionVector *pRefMV; // will be colocated L0 or L1
    Ipp32s MBsCol[4];
    Ipp32s sbColPartOffset[4];
    if (bUseDirect8x8Inference)
    {
        sbColPartOffset[0]=0;
        //sbColPartOffset[1]=3;
        sbColPartOffset[2]=12;
        //sbColPartOffset[3]=15;
        MBsCol[0] = GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[0]);
        MBsCol[1] = MBsCol[0];//GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[1]);
        sbColPartOffset[1] = sbColPartOffset[0]+3;
        MBsCol[2] = GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[2]);
        MBsCol[3] = MBsCol[2];//GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[3]);
        sbColPartOffset[3] = sbColPartOffset[2]+3;
    }
    else
    {
        sbColPartOffset[0]=0;
        //sbColPartOffset[1]=2;
        sbColPartOffset[2]=8;
        //sbColPartOffset[3]=10;
        MBsCol[0] = GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[0]);
        MBsCol[1] = MBsCol[0];//GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[1]);
        sbColPartOffset[1] = sbColPartOffset[0]+2;
        MBsCol[2] = GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[2]);
        MBsCol[3] = MBsCol[2];//GetColocatedLocation(pRefPicList1[0],Field,sbColPartOffset[3]);
        sbColPartOffset[3] = sbColPartOffset[2]+2;
    }

    for (sb=0; sb<4; sb++)
    {
        if ((uSaveMBType != MBTYPE_INTER_8x8 && uSaveMBType != MBTYPE_INTER_8x8_REF0) ||
            m_cur_mb.GlobalMacroblockInfo->sbtype[sb] == SBTYPE_DIRECT)
        {
            // DIRECT 8x8 block
            bAll4x4AreSame = true;

            // 4 4x4 blocks
            for (ypos=0; ypos<2; ypos++)
            {
                for (xpos=0; xpos<2; xpos++)
                {
                    sbcolpartoffset = bUseDirect8x8Inference? sbColPartOffset[sb]:sbColPartOffset[sb]+ypos*4+xpos;
                    Ipp32u MBCol=MBsCol[sb];
                    H264DecoderMotionVector *pRefMVL0 = pRefPicList1[0]->m_mbinfo.MV[0][MBCol].MotionVectors;
                    H264DecoderMotionVector *pRefMVL1 = pRefPicList1[0]->m_mbinfo.MV[1][MBCol].MotionVectors;
                    Ipp8s *pColocatedRefIndexL0 = pRefPicList1[0]->m_mbinfo.RefIdxs[0][MBCol].RefIdxs;
                    Ipp8s *pColocatedRefIndexL1 = pRefPicList1[0]->m_mbinfo.RefIdxs[1][MBCol].RefIdxs;
                    bUseZeroPredL0 = false;
                    bUseZeroPredL1 = false;
                    sbpartoffset = (ypos+(sb&2))*4+(sb&1)*2;

                    if ( ((RefIndexL0 != -1) || (RefIndexL1 != -1)) &&
                        IS_INTER_MBTYPE(pRefPicList1[0]->m_mbinfo.mbs[MBCol].mbtype) &&
                        bL1RefPicisShortTerm)
                    {
                        // When direct 8x8 inference flag is non-zero check the
                        // ref index and motion vector of the outer corner 4x4
                        // block of the colocated 8x8 MB part.

                        // forcing zero pred is possible, check colocated
                        // ref index for 0
                        colocatedRefIndex = pColocatedRefIndexL0[sbcolpartoffset];
                        if (colocatedRefIndex >= 0)
                        {
                            // use L0 colocated
                            pRefMV = pRefMVL0;
                        }
                        else
                        {
                            // use L1 colocated
                            colocatedRefIndex = pColocatedRefIndexL1[sbcolpartoffset];
                            pRefMV = pRefMVL1;
                        }
                        if (colocatedRefIndex == 0)
                        {
                            VM_ASSERT(pRefMV);
                            if (pRefMV[sbcolpartoffset].mvx >= -1 &&
                                pRefMV[sbcolpartoffset].mvx <= 1 &&
                                pRefMV[sbcolpartoffset].mvy >= -1 &&
                                pRefMV[sbcolpartoffset].mvy <= 1)
                            {
                                // All subpart conditions for forcing zero pred are met,
                                // use final RefIndexLx==0 condition for L0,L1.
                                bUseZeroPredL0 = (RefIndexL0 == 0);
                                bUseZeroPredL1 = (RefIndexL1 == 0);
                                bAll4x4AreSame = false;
                                bAll8x8AreSame = false;
                            }
                        }
                    }

                    if ( (RefIndexL0 != -1) && !bUseZeroPredL0)
                    {
                        pFwdMV->MotionVectors[xpos+sbpartoffset].mvx = (Ipp16s) mvxL0;
                        pFwdMV->MotionVectors[xpos+sbpartoffset].mvy = (Ipp16s) mvyL0;
                    }
                    else
                    {
                        pFwdMV->MotionVectors[xpos+sbpartoffset].mvx = 0;
                        pFwdMV->MotionVectors[xpos+sbpartoffset].mvy = 0;
                    }
                    pFwdRefIndex[xpos+sbpartoffset] = bBothRefIdxNoExist ? 0 : RefIndexL0;

                    if ( (RefIndexL1 != -1) && !bUseZeroPredL1)
                    {
                        pBwdMV->MotionVectors[xpos+sbpartoffset].mvx = (Ipp16s) mvxL1;
                        pBwdMV->MotionVectors[xpos+sbpartoffset].mvy = (Ipp16s) mvyL1;
                    }
                    else
                    {
                        pBwdMV->MotionVectors[xpos+sbpartoffset].mvx = 0;
                        pBwdMV->MotionVectors[xpos+sbpartoffset].mvy = 0;
                    }
                    pBwdRefIndex[xpos+sbpartoffset] = bBothRefIdxNoExist ? 0 : RefIndexL1;

                }   // xpos

                // next row of subblocks


            }   // ypos

            // set direction for 8x8 block
            m_cur_mb.LocalMacroblockInfo->sbdir[sb] = (Ipp8u)uPredDir;

            // set type for 8x8 block
            // do not change type from SBTYPE_DIRECT when mbtype is 8x8 because
            // subsequent call to DecodeMotionVectors for the non-DIRECT 8x8 blocks
            // depends upon the SBTYPE_DIRECT value to know to do nothing for those
            // 8x8 blocks.
            if (uSaveMBType != MBTYPE_INTER_8x8 && uSaveMBType != MBTYPE_INTER_8x8_REF0)
            {
                if (bAll4x4AreSame)
                    m_cur_mb.GlobalMacroblockInfo->sbtype[sb] = SBTYPE_8x8;
                else
                    m_cur_mb.GlobalMacroblockInfo->sbtype[sb] = SBTYPE_4x4;
            }
        }   // DIRECT 8x8 block

    }   // for sb

    // set mbtype to 8x8 if it was not; use larger type if possible
    if (uSaveMBType != MBTYPE_INTER_8x8 && uSaveMBType != MBTYPE_INTER_8x8_REF0)
    {
        if (bAll8x8AreSame)
        {
            if (uPredDir == D_DIR_DIRECT_SPATIAL_FWD)
                m_cur_mb.GlobalMacroblockInfo->mbtype= MBTYPE_FORWARD;
            else if (uPredDir == D_DIR_DIRECT_SPATIAL_BWD)
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_BACKWARD;
            else
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_BIDIR;
        }
        else
            m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTER_8x8;
    }

    return ps;

} // Status H264SegmentDecoder::DecodeDirectMotionVectorsSpatial(H264DecoderFrame **pRefPicList0,

} // namespace UMC
