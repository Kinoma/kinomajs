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

///////////////////////////////////////////////////////////////////////////////
// lookup table to translate B frame type code to MB type
const
Ipp8u CodeToMBTypeB[] =
{
    MBTYPE_DIRECT,          // 0
    MBTYPE_FORWARD,
    MBTYPE_BACKWARD,
    MBTYPE_BIDIR,
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,      // 5
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_16x8,      // 10
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,      // 15
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_16x8,
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_16x8,      // 20
    MBTYPE_INTER_8x16,
    MBTYPE_INTER_8x8
};

// lookup table to extract prediction direction from MB type code for
// 16x8 and 8x16 MB types. Contains direction for first and second
// subblocks at each entry.
const
Ipp8u CodeToBDir[][2] =
{
    {D_DIR_FWD, D_DIR_FWD},
    {D_DIR_BWD, D_DIR_BWD},
    {D_DIR_FWD, D_DIR_BWD},
    {D_DIR_BWD, D_DIR_FWD},
    {D_DIR_FWD, D_DIR_BIDIR},
    {D_DIR_BWD, D_DIR_BIDIR},
    {D_DIR_BIDIR, D_DIR_FWD},
    {D_DIR_BIDIR, D_DIR_BWD},
    {D_DIR_BIDIR, D_DIR_BIDIR}
};

// lookup table to translate B frame 8x8 subblock code to type and
// prediction direction
static
const struct
{
    Ipp8u type;
    Ipp8u dir;
} CodeToSBTypeAndDir[] =
{
    {SBTYPE_DIRECT, D_DIR_DIRECT},
    {SBTYPE_8x8, D_DIR_FWD},
    {SBTYPE_8x8, D_DIR_BWD},
    {SBTYPE_8x8, D_DIR_BIDIR},
    {SBTYPE_8x4, D_DIR_FWD},
    {SBTYPE_4x8, D_DIR_FWD},
    {SBTYPE_8x4, D_DIR_BWD},
    {SBTYPE_4x8, D_DIR_BWD},
    {SBTYPE_8x4, D_DIR_BIDIR},
    {SBTYPE_4x8, D_DIR_BIDIR},
    {SBTYPE_4x4, D_DIR_FWD},
    {SBTYPE_4x4, D_DIR_BWD},
    {SBTYPE_4x4, D_DIR_BIDIR}
};

static
Ipp8u ICBPTAB[6] =
{
    0,16,32,15,31,47
};

const
Ipp32s NIT2LIN[16] =
{
    0, 1, 4, 5,
    2, 3, 6, 7,
    8, 9,12,13,
    10,11,14,15
};

// Lookup table to get the 4 bit positions for the 4x4 blocks in the
// blockcbp from the coded bits in 8x8 bitstream cbp.
static
Ipp32u blockcbp_table[5] =
{
    (0xf<<1), (0xf0<<1), (0xf00<<1), (0xf000<<1), (0x30000<<1)
};

/* DEBUG : this table requires to fix */
static
Ipp32u blockcbp_cac_table[] =
{
    (Ipp32u) (0x3ff0000 << 1),
    (Ipp32u) (0x3ff0000 << 1),
    (Ipp32u) (0x3ff0000 << 1),
    (Ipp32u) (0x3ff0000 << 1)
};

// Lookup table to obtain NumCoefToLeft index (0..7) from block number in
// decodeCoefficients, block 0 is INTRA16 DC. Used for luma and chroma.
static const Ipp32u BlockNumToMBRowLuma[17] =
{ 0,0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};
static const Ipp32u BlockNumToMBRowChromaAC[4][32] =
{
    { 0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1},
    { 0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1},
    { 0,0,1,1,2,2,3,3,0,0,1,1,2,2,3,3,0,0,1,1,2,2,3,3,0,0,1,1,2,2,3,3},
    { 0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3,0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3}
};

// Lookup table to obtain NumCoefAbove index (0..7) from block number in
// decodeCoefficients, block 0 is INTRA16 DC. Used for luma and chroma.
static const Ipp32u BlockNumToMBColLuma[17] =
{ 0,0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3};

static const Ipp32u BlockNumToMBColChromaAC[4][32] =
{
    { 0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1},
    { 0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1},
    { 0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1},
    { 0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3}

};

Status H264SegmentDecoder::DecodeMacroBlockType(Ipp32u *pMBIntraTypes,
                                                Ipp32s *MBSkipCount, // On entry if < 0, run of skipped MBs has just been completed
                                                                     // On return, zero or skip MB run count read from bitstream
                                                Ipp32s *PassFDFDecode)

{
    Status status = UMC_OK;
    Ipp32u uCodeNum;

    // interpretation of code depends upon slice type
    if (m_pSliceHeader->slice_type == INTRASLICE)
    {
        {
            pSetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo,0);
        }
        uCodeNum = m_pBitStream->GetVLCElement_unsigned();
        if (uCodeNum == 0)
             m_cur_mb.GlobalMacroblockInfo->mbtype =  MBTYPE_INTRA;
        else if (uCodeNum == 25)
            m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_PCM;
        else
        {
            m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTRA_16x16;
            uCodeNum--;
        }
    }   // intra
    else
    {   // not Intra

        if (*MBSkipCount >= 0) //actually it has to be = 0
        {
            VM_ASSERT(*MBSkipCount<=0);
            uCodeNum = m_pBitStream->GetVLCElement_unsigned();
            // skipped MB count
            if (uCodeNum)
            {
                *PassFDFDecode = 0;
                *MBSkipCount = uCodeNum;
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_SKIPPED;
                goto done;
            }
        }
        else
        {
            // first MB after run of skipped MBs, no new skip count
            // in bitstream to read, clear MBSkipCount to detect next skip run
            *MBSkipCount = 0;
        }

        {
            pSetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo,0);
            *PassFDFDecode = 0;
        }
        uCodeNum = m_pBitStream->GetVLCElement_unsigned();

        if (m_pSliceHeader->slice_type == PREDSLICE)
        {
            switch (uCodeNum)
            {
            case 0:
                // 16x16
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_FORWARD;
                break;
            case 1:
                // 16x8
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTER_16x8;
                break;
            case 2:
                // 8x16
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTER_8x16;
                break;
            case 3:
            case 4:
                // 8x8
                m_cur_mb.GlobalMacroblockInfo->mbtype = (Ipp8u) ((uCodeNum == 4) ? MBTYPE_INTER_8x8_REF0 : MBTYPE_INTER_8x8);
                {
                    // read subblock types
                    Ipp32u subblock;
                    Ipp8u sbtype;

                    for (subblock=0; subblock<4; subblock++)
                    {
                        uCodeNum = m_pBitStream->GetVLCElement_unsigned();
                        switch (uCodeNum)
                        {
                        case 0:
                            sbtype = SBTYPE_8x8;
                            break;
                        case 1:
                            sbtype = SBTYPE_8x4;
                            break;
                        case 2:
                            sbtype = SBTYPE_4x8;
                            break;
                        case 3:
                            sbtype = SBTYPE_4x4;
                            break;
                        default:
                            sbtype = (Ipp8u) -1;
                            status = UMC_BAD_STREAM;
                            break;
                        }
                        m_cur_mb.GlobalMacroblockInfo->sbtype[subblock] = sbtype;

                    }   // for subblock
                }   // 8x8 subblocks
                break;
            case 5:
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTRA;
                break;
            default:
                if (uCodeNum < 30)
                {
                    m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTRA_16x16;
                    uCodeNum -= 6;
                }
                else if (uCodeNum == 30)
                {
                    m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_PCM;
                }
                else
                {
                    status = UMC_BAD_STREAM;
                }
                break;
            }
        }   // P frame
        else if (m_pSliceHeader->slice_type == BPREDSLICE)
        {
            if (uCodeNum < 23)
            {
                m_cur_mb.GlobalMacroblockInfo->mbtype = CodeToMBTypeB[uCodeNum];
                if (m_cur_mb.GlobalMacroblockInfo->mbtype  == MBTYPE_INTER_16x8 ||
                    m_cur_mb.GlobalMacroblockInfo->mbtype == MBTYPE_INTER_8x16)
                {
                    // direction for the two subblocks
                    m_cur_mb.LocalMacroblockInfo->sbdir[0] = CodeToBDir[(uCodeNum-4)>>1][0];
                    m_cur_mb.LocalMacroblockInfo->sbdir[1] = CodeToBDir[(uCodeNum-4)>>1][1];
                }
                if (m_cur_mb.GlobalMacroblockInfo->mbtype  == MBTYPE_INTER_8x8 || m_cur_mb.GlobalMacroblockInfo->mbtype  == MBTYPE_INTER_8x8_REF0)
                {
                    // read subblock types and prediction direction
                    Ipp32u subblock;
                    for (subblock=0; subblock<4; subblock++)
                    {
                        uCodeNum = m_pBitStream->GetVLCElement_unsigned();
                        if (uCodeNum < 13)
                        {
                            m_cur_mb.GlobalMacroblockInfo->sbtype[subblock] =  CodeToSBTypeAndDir[uCodeNum].type;
                            m_cur_mb.LocalMacroblockInfo->sbdir[subblock] = CodeToSBTypeAndDir[uCodeNum].dir;
                        }
                        else
                        {
                            status = UMC_BAD_STREAM;
                        }
                    }   // for subblock
                }   // 8x8 subblocks
            }
            else if (uCodeNum == 23)
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTRA;
            else if (uCodeNum < 48)
            {
                    m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_INTRA_16x16;
                    uCodeNum -= 24;
            }
            else if (uCodeNum == 48)
            {
                m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_PCM;
            }
            else
            {
                status = UMC_BAD_STREAM;
            }
        }   // B frame
        else
        {
            status = UMC_BAD_STREAM;
        }


    }   // not Intra

	//***kinoma enhancement -bnie 8/3/2008
	CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE

    if (m_cur_mb.GlobalMacroblockInfo->mbtype == MBTYPE_INTRA_16x16)
    {
        // 16x16 INTRA, code includes prediction mode and cbp info

        m_cur_mb.LocalMacroblockInfo->cbp = ICBPTAB[(uCodeNum)>>2];
        pMBIntraTypes[0] =
        pMBIntraTypes[1] =
        pMBIntraTypes[2] =
        pMBIntraTypes[3] = (uCodeNum) & 0x03;
            //*pMBIntraTypes = (Ipp8u)(uCodeNum & 3); // 0..3, mode
            if (uCodeNum > 11)
            {
                m_cur_mb.LocalMacroblockInfo->cbp4x4_luma |= D_CBP_LUMA_AC;// at least one luma AC coeff. present
                uCodeNum -= 12; // for chroma cbp extraction
            }
            uCodeNum >>= 2;
            if (uCodeNum > 0)
            {
                m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[0] |= D_CBP_CHROMA_DC;
                m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[1] |= D_CBP_CHROMA_DC;

                if (uCodeNum > 1)
                {
                    m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[0] |= D_CBP_CHROMA_AC;
                    m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[1] |= D_CBP_CHROMA_AC;
                }
            }
    }   // INTRA_16x16
done:
    m_cur_mb.LocalMacroblockInfo->mbtypeBS = m_cur_mb.GlobalMacroblockInfo->mbtype;

//***kinoma enhancement  bnie 8/3/2008
packet_loss_happened:

    return status;

} // Status H264SegmentDecoder::DecodeMacroBlockType(Ipp32u *pMBIntraTypes,

Status H264SegmentDecoder::DecodeIntraTypes4x4_CAVLC(Ipp32u *pMBIntraTypes,
                                                     bool bUseConstrainedIntra)
{
    Ipp32u block;
//    Ipp32u mb_width = m_pCurrentFrame->macroBlockSize().width;
    //Ipp32s mb_left_offset = -(mb_width*pMB->bottom_mb)-1;
    //Ipp32s mb_above_offset = -(mb_width*pMB->bottom_mb)-mb_width;
    // Temp arrays for modes from above and left, initially filled from
    // outside the MB, then updated with modes within the MB
    Ipp32u dummyAbove;
    Ipp32u dummyLeft;
    Ipp8u *uModeAbove = (Ipp8u*)&dummyAbove;
    Ipp8u *uModeLeft  = (Ipp8u*)&dummyLeft;
    Ipp8u uPredMode;        // predicted mode for current 4x4 block
    Ipp8u uBSMode;          // mode bits from bitstream

    Ipp32u *pRefIntraTypes;
    Ipp32u uLeftIndex;      // indexes into mode arrays, dependent on 8x8 block
    Ipp32u uAboveIndex;
    H264DecoderMacroblockGlobalInfo *gmbinfo=m_pCurrentFrame->m_mbinfo.mbs;
    Ipp32u predictors=31;//5 lsb bits set
    //new version
    {
        // above, left MB available only if they are INTRA
        if ((m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~1);//clear 1-st bit
        if ((m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~2); //clear 2-nd bit
        if ((m_cur_mb.CurrentBlockNeighbours.mbs_left[1].mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[1].mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~4); //clear 3-rd bit
        if ((m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~8); //clear 4-th bit
        if ((m_cur_mb.CurrentBlockNeighbours.mbs_left[3].mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[3].mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~16); //clear 5-th bit
    }

    // Get modes of blocks above and to the left, substituting 0
    // when above or to left is outside this MB slice. Substitute mode 2
    // when the adjacent macroblock is not 4x4 INTRA. Add 1 to actual
    // modes, so mode range is 1..9.

    if (predictors&1)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].mbtype == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num * NUM_INTRA_TYPE_ELEMENTS;
            uModeAbove[0] = (Ipp8u) (pRefIntraTypes[10] + 1);
            uModeAbove[1] = (Ipp8u) (pRefIntraTypes[11] + 1);
            uModeAbove[2] = (Ipp8u) (pRefIntraTypes[14] + 1);
            uModeAbove[3] = (Ipp8u) (pRefIntraTypes[15] + 1);
        }
        else
        {
            // MB above in slice but not INTRA, use mode 2 (+1)
            //uModeAbove[0]=uModeAbove[1]=uModeAbove[2]=uModeAbove[3] = 2 + 1;
            dummyAbove = 0x03030303;
        }
    }
    else
    {
        //uModeAbove[0]=uModeAbove[1]=uModeAbove[2]=uModeAbove[3] = 0;
        dummyAbove = 0;
    }
    if (predictors&2)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].mbtype  == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num*NUM_INTRA_TYPE_ELEMENTS;
            uModeLeft[0] = (Ipp8u) (pRefIntraTypes[NIT2LIN[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].block_num]] + 1);
        }
        else
        {
            // MB left in slice but not INTRA, use mode 2 (+1)
            uModeLeft[0] = 2+1;
        }
    }
    else
    {
        uModeLeft[0] = 0;
    }

    if (predictors&4)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[1].mb_num].mbtype == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mbs_left[1].mb_num*NUM_INTRA_TYPE_ELEMENTS;
            uModeLeft[1] = (Ipp8u) (pRefIntraTypes[NIT2LIN[m_cur_mb.CurrentBlockNeighbours.mbs_left[1].block_num]] + 1);
        }
        else
        {
            // MB left in slice but not INTRA, use mode 2 (+1)
            uModeLeft[1] = 2+1;
        }
    }
    else
    {
        uModeLeft[1] = 0;
    }
    if (predictors&8)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num].mbtype == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num*NUM_INTRA_TYPE_ELEMENTS;
            uModeLeft[2] = (Ipp8u) (pRefIntraTypes[NIT2LIN[m_cur_mb.CurrentBlockNeighbours.mbs_left[2].block_num]] + 1);
        }
        else
        {
            // MB left in slice but not INTRA, use mode 2 (+1)
            uModeLeft[2] = 2+1;
        }
    }
    else
    {
        uModeLeft[2] = 0;
    }

    if (predictors&16)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[3].mb_num].mbtype  == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mbs_left[3].mb_num*NUM_INTRA_TYPE_ELEMENTS;
            uModeLeft[3] = (Ipp8u) (pRefIntraTypes[NIT2LIN[m_cur_mb.CurrentBlockNeighbours.mbs_left[3].block_num]] + 1);
        }
        else
        {
            // MB left in slice but not INTRA, use mode 2 (+1)
            uModeLeft[3] = 2+1;
        }
    }
    else
    {
        uModeLeft[3] = 0;
    }
    for (block=0; block<4; block++)
    {
        uAboveIndex = (block & 1) * 2;      // 0,2,0,2
        uLeftIndex = (block & 2);           // 0,0,2,2

        // upper left 4x4

        // Predicted mode is minimum of the above and left modes, or
        // mode 2 if above or left is outside slice, indicated by 0 in
        // mode array.
        uPredMode = MIN(uModeLeft[uLeftIndex], uModeAbove[uAboveIndex]);
        if (uPredMode)
            uPredMode--;
        else
            uPredMode = 2;

        // If next bitstream bit is 1, use predicted mode, else read new mode
        if (m_pBitStream->Get1Bit() == 0)
        {
            // get 3 more bits to determine new mode
            uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
            if (uBSMode < uPredMode)
                uPredMode = uBSMode;
            else
                uPredMode = uBSMode + 1;
        }
        // Save mode
        pMBIntraTypes[0] = uPredMode;
        uModeAbove[uAboveIndex] = uPredMode + 1;

        // upper right 4x4
        uPredMode = MIN(uPredMode+1, uModeAbove[uAboveIndex+1]);
        if (uPredMode)
            uPredMode--;
        else
            uPredMode = 2;

        if (m_pBitStream->Get1Bit() == 0)
        {
            uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
            if (uBSMode < uPredMode)
                uPredMode = uBSMode;
            else
                uPredMode = uBSMode + 1;
        }

        pMBIntraTypes[1] = uPredMode;
        uModeAbove[uAboveIndex+1] = uPredMode + 1;
        uModeLeft[uLeftIndex] = uPredMode + 1;

        // lower left 4x4
        uPredMode = MIN(uModeLeft[uLeftIndex+1], uModeAbove[uAboveIndex]);
        if (uPredMode)
            uPredMode--;
        else
            uPredMode = 2;

        if (m_pBitStream->Get1Bit() == 0)
        {
            uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
            if (uBSMode < uPredMode)
                uPredMode = uBSMode;
            else
                uPredMode = uBSMode + 1;
        }
        pMBIntraTypes[2] = uPredMode;
        uModeAbove[uAboveIndex] = uPredMode + 1;

        // lower right 4x4 (above and left must always both be in slice)
        uPredMode = MIN(uPredMode+1, uModeAbove[uAboveIndex+1]) - 1;

        if (m_pBitStream->Get1Bit() == 0)
        {
            uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
            if (uBSMode < uPredMode)
                uPredMode = uBSMode;
            else
                uPredMode = uBSMode + 1;
        }
        pMBIntraTypes[3]             = uPredMode;
        uModeAbove   [uAboveIndex+1] = uPredMode + 1;
        uModeLeft    [uLeftIndex+1]  = uPredMode + 1;

        pMBIntraTypes += 4;
    }   // block

    return UMC_OK;

} // Status H264SegmentDecoder::DecodeIntraTypes4x4_CAVLC(Ipp32u *pMBIntraTypes,

Status H264SegmentDecoder::DecodeIntraTypes8x8_CAVLC(Ipp32u *pMBIntraTypes,
                                                   bool bUseConstrainedIntra)
{
    Ipp16u dummyAbove;
    Ipp16u dummyLeft;
    Ipp8u *uModeAbove = (Ipp8u*)&dummyAbove;
    Ipp8u *uModeLeft  = (Ipp8u*)&dummyLeft;
    Ipp8u uPredMode;        // predicted mode for current 4x4 block
    Ipp8u uBSMode;          // mode bits from bitstream

    Ipp32u *pRefIntraTypes;
    Ipp32u uLeftIndex;      // indexes into mode arrays, dependent on 8x8 block
    Ipp32u uAboveIndex;
    H264DecoderMacroblockGlobalInfo *gmbinfo=m_pCurrentFrame->m_mbinfo.mbs;
    Ipp32u predictors=31;//5 lsb bits set
    //new version
    {
        // above, left MB available only if they are INTRA
        if ((m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~1);//clear 1-st bit
        if ((m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~2); //clear 2-nd bit
        if ((m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num<0) || ((!IS_INTRA_MBTYPE(gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num].mbtype) && bUseConstrainedIntra)))
            predictors &= (~4); //clear 4-th bit
    }

    // Get modes of blocks above and to the left, substituting 0
    // when above or to left is outside this MB slice. Substitute mode 2
    // when the adjacent macroblock is not 4x4 INTRA. Add 1 to actual
    // modes, so mode range is 1..9.

    if (predictors&1)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].mbtype == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num * NUM_INTRA_TYPE_ELEMENTS;
            uModeAbove[0] = (Ipp8u) (pRefIntraTypes[10] + 1);
            uModeAbove[1] = (Ipp8u) (pRefIntraTypes[14] + 1);
        }
        else
        {
            // MB above in slice but not INTRA, use mode 2 (+1)
            //uModeAbove[0]=uModeAbove[1]=uModeAbove[2]=uModeAbove[3] = 2 + 1;
            dummyAbove = 0x0303;
        }
    }
    else
    {
        //uModeAbove[0]=uModeAbove[1]=uModeAbove[2]=uModeAbove[3] = 0;
        dummyAbove = 0;
    }
    if (predictors&2)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].mbtype  == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num*NUM_INTRA_TYPE_ELEMENTS;
            uModeLeft[0] = (Ipp8u) (pRefIntraTypes[NIT2LIN[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].block_num]] + 1);
        }
        else
        {
            // MB left in slice but not INTRA, use mode 2 (+1)
            uModeLeft[0] = 2+1;
        }
    }
    else
    {
        uModeLeft[0] = 0;
    }

    if (predictors&4)
    {
        if (gmbinfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num].mbtype == MBTYPE_INTRA)
        {
            pRefIntraTypes = m_pMBIntraTypes + m_cur_mb.CurrentBlockNeighbours.mbs_left[2].mb_num*NUM_INTRA_TYPE_ELEMENTS;
            uModeLeft[1] = (Ipp8u) (pRefIntraTypes[NIT2LIN[m_cur_mb.CurrentBlockNeighbours.mbs_left[2].block_num]] + 1);
        }
        else
        {
            // MB left in slice but not INTRA, use mode 2 (+1)
            uModeLeft[1] = 2+1;
        }
    }
    else
    {
        uModeLeft[1] = 0;
    }

    uAboveIndex = 0;
    uLeftIndex = 0;

    // upper left 8x8

    // Predicted mode is minimum of the above and left modes, or
    // mode 2 if above or left is outside slice, indicated by 0 in
    // mode array.
    uPredMode = MIN(uModeLeft[uLeftIndex], uModeAbove[uAboveIndex]);
    if (uPredMode)
        uPredMode--;
    else
        uPredMode = 2;

    // If next bitstream bit is 1, use predicted mode, else read new mode
    if (m_pBitStream->Get1Bit() == 0)
    {
        // get 3 more bits to determine new mode
        uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
        if (uBSMode < uPredMode)
            uPredMode = uBSMode;
        else
            uPredMode = uBSMode + 1;
    }
    // Save mode
    pMBIntraTypes[0] =
    pMBIntraTypes[1] =
    pMBIntraTypes[2] =
    pMBIntraTypes[3] =
        uPredMode;
    uModeAbove[uAboveIndex] = uPredMode + 1;

    // upper right 8x8
    uPredMode = MIN(uPredMode+1, uModeAbove[uAboveIndex+1]);
    if (uPredMode)
        uPredMode--;
    else
        uPredMode = 2;

    if (m_pBitStream->Get1Bit() == 0)
    {
        uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
        if (uBSMode < uPredMode)
            uPredMode = uBSMode;
        else
            uPredMode = uBSMode + 1;
    }

    pMBIntraTypes[4] =
    pMBIntraTypes[5] =
    pMBIntraTypes[6] =
    pMBIntraTypes[7] =
        uPredMode;
    uModeAbove[uAboveIndex+1] = uPredMode + 1;
    uModeLeft[uLeftIndex] = uPredMode + 1;

    // lower left 4x4
    uPredMode = MIN(uModeLeft[uLeftIndex+1], uModeAbove[uAboveIndex]);
    if (uPredMode)
        uPredMode--;
    else
        uPredMode = 2;

    if (m_pBitStream->Get1Bit() == 0)
    {
        uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
        if (uBSMode < uPredMode)
            uPredMode = uBSMode;
        else
            uPredMode = uBSMode + 1;
    }
    pMBIntraTypes[8] =
    pMBIntraTypes[9] =
    pMBIntraTypes[10] =
    pMBIntraTypes[11] =
     uPredMode;
    uModeAbove[uAboveIndex] = uPredMode + 1;

    // lower right 4x4 (above and left must always both be in slice)
    uPredMode = MIN(uPredMode+1, uModeAbove[uAboveIndex+1]) - 1;

    if (m_pBitStream->Get1Bit() == 0)
    {
        uBSMode = (Ipp8u)m_pBitStream->GetBits(3);
        if (uBSMode < uPredMode)
            uPredMode = uBSMode;
        else
            uPredMode = uBSMode + 1;
    }
    pMBIntraTypes[12] =
    pMBIntraTypes[13] =
    pMBIntraTypes[14] =
    pMBIntraTypes[15] =
        uPredMode;
    uModeAbove   [uAboveIndex+1] = uPredMode + 1;
    uModeLeft    [uLeftIndex+1]  = uPredMode + 1;


    // copy last IntraTypes to first 4 for reconstruction since they're not used for further prediction
    pMBIntraTypes[1] = pMBIntraTypes[4];
    pMBIntraTypes[2] = pMBIntraTypes[8];
    pMBIntraTypes[3] = pMBIntraTypes[12];

    return UMC_OK;

} // Status H264SegmentDecoder::DecodeIntraTypes8x8_CAVLC(Ipp32u *pMBIntraTypes,

Ipp8u H264SegmentDecoder::DecodeCBP_CAVLC(Ipp32u mbtype,
                                          Ipp8u color_format)
{
    Ipp8u index;
    Ipp8u cbp = 255;

    index = (Ipp8u) m_pBitStream->GetVLCElement_unsigned();

    if (index < 48)
    {
        if (mbtype==MBTYPE_INTRA)
            cbp = dec_cbp_intra[0 != color_format][index];
        else
            cbp = dec_cbp_inter[0 != color_format][index];
    }

    return cbp;

} // Ipp8u H264SegmentDecoder::DecodeCBP_CAVLC(Ipp32u mbtype,

Status H264SegmentDecoder::DecodeCoeffs16x16BMEH_CAVLC()
{
    Status umcRes;

    // decode luma DC block
    {
        Ipp16s sNumCoeff;
        Ipp32u uNC = 0;

        uNC = GetDCBlocksLumaContext();
        umcRes = m_pBitStream->GetCAVLCInfoLuma(uNC,
                                                16,
                                                sNumCoeff,
                                                m_pCoeffBlocksWrite,
                                                -1);
        if (UMC_OK != umcRes)
            return umcRes;

        if (0 == sNumCoeff)
            m_cur_mb.LocalMacroblockInfo->cbp4x4_luma &= ~D_CBP_LUMA_DC;
        else
            m_cur_mb.LocalMacroblockInfo->cbp4x4_luma |= D_CBP_LUMA_DC;
    }

    // decode luma AC blocks
    if (m_cur_mb.LocalMacroblockInfo->cbp4x4_luma & D_CBP_LUMA_AC)
    {
        Ipp32u uCurBlock = 2;
        Ipp32u uLumaCBP = m_cur_mb.LocalMacroblockInfo->cbp4x4_luma;
        Ipp32s iBlock = FIRST_AC_LUMA;

        while (uCurBlock <= uLumaCBP)
        {
            Ipp32u uAboveIndex;
            Ipp32u uLeftIndex;
            Ipp16s sNumCoeff = 0;

            uAboveIndex = BlockNumToMBColLuma[iBlock];
            uLeftIndex = BlockNumToMBRowLuma[iBlock];

            if (uCurBlock & uLumaCBP)
            {
                Ipp32u uNC = 0;

                uNC = GetBlocksLumaContext(uAboveIndex, uLeftIndex);

                // get CAVLC-code coefficient info from bitstream. following call
                // updates pbs, bitOffset, sNumCoeff, sNumTrOnes, TrOneSigns,
                // and uTotalZero and fills CoeffBuf and uRunBeforeBuf.
                umcRes = m_pBitStream->GetCAVLCInfoLuma(uNC,
                                                        15,
                                                        sNumCoeff,
                                                        m_pCoeffBlocksWrite,
                                                        -1);
                if (umcRes != UMC_OK)
                    return umcRes;

                if (0 == sNumCoeff)
                    uLumaCBP ^= uCurBlock;
            }

            // update num coeff storage for predicting future blocks
            m_cur_mb.MacroblockCoeffsInfo->numCoeff[uLeftIndex * 4 + uAboveIndex] = (Ipp8u) sNumCoeff;

            uCurBlock <<= 1;
            iBlock += 1;
        }

        // update luma coded block pattern
        m_cur_mb.LocalMacroblockInfo->cbp4x4_luma = uLumaCBP;
    }

    // decode chroma DC blocks
    {
        Ipp32s iComponent;

        for(iComponent = 0; iComponent < 2; iComponent += 1)
        {
            if (m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[iComponent] & D_CBP_CHROMA_DC)
            {
                Ipp16s sNumCoeff;

                umcRes = m_pBitStream->GetCAVLCInfoChroma0(sNumCoeff, m_pCoeffBlocksWrite);
                if (UMC_OK != umcRes)
                    return umcRes;

                if (0 == sNumCoeff)
                    m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[iComponent] &= ~D_CBP_CHROMA_DC;
                else
                    m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[iComponent] |= D_CBP_CHROMA_DC;
            }
        }
    }

    // decode chroma AC blocks
    if ((m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[0] & D_CBP_CHROMA_AC) ||
        (m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[1] & D_CBP_CHROMA_AC))
    {
        Ipp32s iComponent;

        for (iComponent = 0; iComponent < 2; iComponent += 1)
        {
            Ipp32s iBlock = FIRST_AC_CHROMA + iComponent * 4;
            Ipp32u uCurBlock = 2;
            Ipp32u uChromaCBP = m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[iComponent];
            Ipp32u addval = (iComponent) ? (20) : (16);

            while (uCurBlock <= uChromaCBP)
            {
                Ipp32u uAboveIndex;
                Ipp32u uLeftIndex;
                Ipp16s sNumCoeff = 0;

                uAboveIndex = BlockNumToMBColChromaAC[1][iBlock - FIRST_AC_CHROMA];
                uLeftIndex  = BlockNumToMBRowChromaAC[1][iBlock - FIRST_AC_CHROMA];

                if (uCurBlock & uChromaCBP)
                {
                    Ipp32u uNC = 0;

                    uNC = GetBlocksChromaContextBMEH(uAboveIndex, uLeftIndex, iComponent);

                    // get CAVLC-code coefficient info from bitstream. following call
                    // updates pbs, bitOffset, sNumCoeff, sNumTrOnes, TrOneSigns,
                    // and uTotalZero and fills CoeffBuf and uRunBeforeBuf.
                    umcRes = m_pBitStream->GetCAVLCInfoLuma(uNC,
                                                            15,
                                                            sNumCoeff,
                                                            m_pCoeffBlocksWrite,
                                                            -1);
                    if (UMC_OK != umcRes)
                        return umcRes;

                    if (0 == sNumCoeff)
                        uChromaCBP ^= uCurBlock;
                }

                // update num coeff storage for predicting future blocks
                m_cur_mb.MacroblockCoeffsInfo->numCoeff[uLeftIndex * 2 + uAboveIndex + addval] = (Ipp8u) sNumCoeff;

                uCurBlock <<= 1;
                iBlock += 1;
            }

            // update chroma coded block pattern
            m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[iComponent] = uChromaCBP;
        }
    }

    return UMC_OK;

} // Status H264SegmentDecoder::DecodeCoeffsIntra16x16_CAVLC(Ipp32u)

Status H264SegmentDecoder::DecodeCoeffs4x4BMEH_CAVLC()
{
    Status ps = UMC_OK;
    Ipp32u  blockcbp;
    Ipp32u  u8x8block = 1;
    Ipp32u  uBlock;
    Ipp32u  uBlockBit;

    Ipp32u  uNC;
    Ipp32u  uAboveIndex;
    Ipp32u  uLeftIndex;

    Ipp16s  sNumCoeff;
    Ipp8u cbp = m_cur_mb.LocalMacroblockInfo->cbp;

    // Initialize blockcbp bits from input cbp (from the bitstream)
    blockcbp = 0;   // no coeffs
    for (uBlock=0; uBlock<5; uBlock++)
    {
        if (cbp & u8x8block)
            blockcbp |= blockcbp_table[uBlock];
        u8x8block <<= 1;
    }

    if (cbp & u8x8block)
        blockcbp |= blockcbp_cac_table[1];

    uBlock      = 1;        // start block loop with first luma 4x4
    uBlockBit   = 2;
    blockcbp  >>= 1;

    for (uBlock = 1; uBlock < FIRST_DC_CHROMA; uBlock++)
    {
        uAboveIndex = BlockNumToMBColLuma[uBlock];
        uLeftIndex  = BlockNumToMBRowLuma[uBlock];

        sNumCoeff = 0;
        if ((blockcbp & 1) != 0)
        {
            uNC=GetBlocksLumaContext(uAboveIndex,uLeftIndex);

            // Get CAVLC-code coefficient info from bitstream. Following call
            // updates pbs, bitOffset, sNumCoeff, sNumTrOnes, TrOneSigns,
            // and uTotalZero and fills CoeffBuf and uRunBeforeBuf.
            ps = m_pBitStream->GetCAVLCInfoLuma(uNC,
                                                16,
                                                sNumCoeff,
                                                m_pCoeffBlocksWrite,
                                                -1);
            if (ps != UMC_OK)
                return ps;

            m_cur_mb.LocalMacroblockInfo->cbp4x4_luma |= (sNumCoeff ? uBlockBit : 0);
        }

        // Update num coeff storage for predicting future blocks
        m_cur_mb.MacroblockCoeffsInfo->numCoeff[uLeftIndex * 4 + uAboveIndex] = (Ipp8u) sNumCoeff;

        blockcbp >>= 1;
        uBlockBit <<= 1;
    }   // uBlock

    for (uBlock = FIRST_DC_CHROMA; uBlock < FIRST_AC_CHROMA; uBlock++)
    {
        if ((blockcbp & 1) != 0)
        {
            ps = m_pBitStream->GetCAVLCInfoChroma0 ( sNumCoeff, m_pCoeffBlocksWrite);
            if (ps != UMC_OK)
                return ps;

            m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[uBlock - FIRST_DC_CHROMA] |= (sNumCoeff ? 1 : 0);
        }

        blockcbp >>= 1;
        //  Can't early exit without setting numcoeff for rest of blocks
        if ((blockcbp == 0) && (uBlock == (FIRST_AC_CHROMA - 1)))
        {
            // no AC chroma coeffs, set chrroma NumCoef buffers to zero and exit
            //pMB->numCoeff[16]=pMB->numCoeff[17]=pMB->numCoeff[18]=pMB->numCoeff[19]=
            //pMB->numCoeff[20]=pMB->numCoeff[21]=pMB->numCoeff[22]=pMB->numCoeff[23]=0;

            goto done;
        }
    }   // uBlock

    uBlockBit = 2;
    for (uBlock = FIRST_AC_CHROMA; uBlock <= 26; uBlock++)
    {
        if (uBlock == 23)
            uBlockBit = 2;

        uAboveIndex = BlockNumToMBColChromaAC[1][uBlock-FIRST_AC_CHROMA];
        uLeftIndex  = BlockNumToMBRowChromaAC[1][uBlock-FIRST_AC_CHROMA];
        Ipp32u addval = uBlock >= 23?20:16;
        sNumCoeff = 0;
        if ((blockcbp & 1) != 0)
        {
            uNC=GetBlocksChromaContextBMEH(uAboveIndex,uLeftIndex,uBlock >= 23);

            // Get CAVLC-code coefficient info from bitstream. Following call
            // updates pbs, bitOffset, sNumCoeff, sNumTrOnes, TrOneSigns,
            // and uTotalZero and fills CoeffBuf and uRunBeforeBuf.
            ps = m_pBitStream->GetCAVLCInfoLuma(uNC,
                                                15,
                                                sNumCoeff,
                                                m_pCoeffBlocksWrite,
                                                -1);
            if (ps != UMC_OK)
                return ps;

            m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[uBlock >= 23] |= (sNumCoeff ? uBlockBit : 0);
        }
        // Update num coeff storage for predicting future blocks
        m_cur_mb.MacroblockCoeffsInfo->numCoeff[uLeftIndex * 2 + uAboveIndex + addval] = (Ipp8u)sNumCoeff;


        blockcbp >>= 1;
        uBlockBit <<= 1;
    }   // uBlock


    // update buffer position pointer
done:

    return ps;

} // Status H264SegmentDecoder::DecodeCoeffs4x4_CAVLC(Ipp32u cbp)


} // namespace UMC
