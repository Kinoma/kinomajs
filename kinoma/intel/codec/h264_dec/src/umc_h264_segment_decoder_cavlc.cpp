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

namespace UMC
{

Status H264SegmentDecoder::DecodeSegmentCAVLC(Ipp32s *pnCurMBNumber, Ipp32s nMacroBlocksToDecode)
{
    Status status = UMC_OK;

    // inter-macroblock variables
    Ipp32s quant_prev/* = m_pSliceStream->m_nQuantPrev*/;
    Ipp32s MBSkipCount/* = m_pSliceStream->m_nMBSkipCount*/;
    Ipp32s PassFDFDecode/* = m_pSliceStream->m_nPassFDFDecode*/;
    m_pSlice->GetStateVariables(MBSkipCount, quant_prev, PassFDFDecode);

    // per-macroblock variables
    Ipp32u nCurMBNumber;
    Ipp8u mbtype = 0;
    bool bIsBSlice;
    bool bClearMV;
    Ipp32u *pMBIntraTypes;
    Ipp32u mbXOffset, mbYOffset;
    Ipp16s SliceNum;
    bool bUseConstrainedIntra;
    bool bUseDirect8x8Inference;
    H264DecoderMBAddr NextMB;

    Ipp32s QPChromaIndex = 0;
    Ipp8u QPChroma = 0;

    SliceNum = (Ipp16s) m_pSlice->GetSliceNum();

    // Reset buffer pointers to start
    bIsBSlice = (m_pSliceHeader->slice_type == BPREDSLICE);

    bUseConstrainedIntra = m_pPicParamSet->constrained_intra_pred_flag != 0;

    bUseDirect8x8Inference = m_pSeqParamSet->direct_8x8_inference_flag != 0;

    H264VideoDecoder::H264DecoderFrame  **pRefPicList0;
    H264VideoDecoder::H264DecoderFrame  **pRefPicList1;

	//***kinoma enhancement   --bnie 8/2/2008
	pRefPicList0 = m_pCurrentFrame->GetRefPicListSafe( SliceNum, 0 );
	pRefPicList1 = m_pCurrentFrame->GetRefPicListSafe( SliceNum, 1 );
    //pRefPicList0 = m_pCurrentFrame->GetRefPicList(SliceNum, 0)->m_RefPicList;
    //pRefPicList1 = m_pCurrentFrame->GetRefPicList(SliceNum, 1)->m_RefPicList;
    Ipp8s          pFields_stub=0;

    m_CurMBAddr = *pnCurMBNumber;
    //***bnie: bool mbaff = (0 != m_pSliceHeader->MbaffFrameFlag);

// reconstruct Data
    IppStatus          sts        = ippStsNoErr;
    bool                intra;
    bool                intra16x16;

    Ipp8u              *pYPlane, *pUPlane, *pVPlane;
    Ipp32u              uPitch = m_pCurrentFrame->pitch();
    Ipp32u              offsetY, offsetC;
    Ipp32s              ChromaQPOffset = m_pPicParamSet->chroma_qp_index_offset[0];
    Ipp32u unumMBs = m_pSlice->GetMaxMB();
    // Current plane pointers
    pYPlane = m_pCurrentFrame->m_pYPlane;
    pUPlane = m_pCurrentFrame->m_pUPlane;
    pVPlane = m_pCurrentFrame->m_pVPlane;

    nCurMBNumber = *pnCurMBNumber;
    {
        Ipp32u nBorder;

        // calculate last MB in row
        nBorder = min(unumMBs, nCurMBNumber + nMacroBlocksToDecode);

        // this cicle for MBs in single row
        for (; nCurMBNumber < nBorder; nCurMBNumber += 1)
        {
			//***kinoma enhancement   --bnie 6/12/2008
			CHECK_PACKET_LOSS_IN_NORMAL_MODE
			
			// Reset buffer pointers to start
            m_pCoeffBlocksWrite = GetCoefficientsBuffer();
            // Reset buffer pointers to start
            // This works only as long as "batch size" for VLD and reconstruct
            // is the same. When/if want to make them different, need to change this.
            m_pCoeffBlocksRead = GetCoefficientsBuffer();
            pMBIntraTypes = m_pMBIntraTypes + m_CurMBAddr*NUM_INTRA_TYPE_ELEMENTS;
            // pel position of current macroblock in the luma plane
            m_CurMB_X = (m_CurMBAddr % mb_width);
            m_CurMB_Y = (m_CurMBAddr / mb_width);
            mbXOffset = m_CurMB_X * 16;
            mbYOffset = m_CurMB_Y * 16;
            VM_ASSERT(mbXOffset < m_pCurrentFrame->lumaSize().width);
            VM_ASSERT(mbYOffset < m_pCurrentFrame->lumaSize().height);
            UpdateCurrentMBInfo();

            // MV and RefIndex store for subblock 0 of the current macroblock.

            bClearMV = true;
            //bClearNC = true;
            NextMB = m_mbinfo.active_next_mb_table[m_CurMBAddr];
            m_cur_mb.LocalMacroblockInfo->QP = (Ipp8s) quant_prev;
            m_cur_mb.GlobalMacroblockInfo->slice_id = (Ipp16s) SliceNum;
            m_cur_mb.LocalMacroblockInfo->cbp4x4_luma =
            m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[0] =
            m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[1] =
            m_cur_mb.LocalMacroblockInfo->cbp =
            m_cur_mb.LocalMacroblockInfo->intra_chroma_mode = 0;
            m_cur_mb.LocalMacroblockInfo->sbdir[0] =
            m_cur_mb.LocalMacroblockInfo->sbdir[1] =
            m_cur_mb.LocalMacroblockInfo->sbdir[2] =
            m_cur_mb.LocalMacroblockInfo->sbdir[3] =D_DIR_FWD;

            // Assume all subblocks have no transmitted coefficients initially.
            // Update cbp4x4 when we see a coefficient.

            UpdateNeighbouringAddresses();

            // Set prediction direction for all partitions as forward, so it
            // is correctly set for SKIPPED and P frame MB for motion comp.

            {
                // Set L1 refidx for all blocks to default value of -1 (no reference).
                // For any bidir blocks, code below sets it to correct refidx. The
                // init to default is required even in non-B slices so the refidx
                // is correctly set for the loop filter for a picture containing
                // both P and B slices.


                Ipp32u *pRefIndexTmp = (Ipp32u *)m_cur_mb.RefIdxs[1]->RefIdxs;

                // pRefIndexTmp is 4-byte aligned for a MB, mb_width=(uSubBlockWidth>>2)
                pRefIndexTmp[0] = 0xffffffff;
                pRefIndexTmp[1] = 0xffffffff;
                pRefIndexTmp[2] = 0xffffffff;
                pRefIndexTmp[3] = 0xffffffff;
            }

            if (MBSkipCount <= 0)
            {
                status = UMC_OK;
                while (UMC_OK == status) // not really a loop, while used to enable use of break
                {
                    // First decode the "macroblock header", e.g., macroblock type,
                    // intra-coding types, motion vectors and CBP.

                    // Get MB type, possibly change MBSKipCount to non-zero
					//***kinoma enhancement   --bnie  8/2/2008
					CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE
                    status = DecodeMacroBlockType(/*pMB,*/ pMBIntraTypes, &MBSkipCount, &PassFDFDecode);

					UpdateNeighbouringBlocksBMEH();//new version
                    if (status != UMC_OK)
                        break;

                    mbtype = m_cur_mb.GlobalMacroblockInfo->mbtype;// pMB->mbtype;

					if (mbtype == MBTYPE_SKIPPED)  break;
                    if (mbtype == MBTYPE_INTRA)
                    {
                        status = DecodeIntraTypes4x4_CAVLC(/*pMB,*/ pMBIntraTypes, bUseConstrainedIntra);
                        if (status != UMC_OK)
                            break;

						//***kinoma enhancement   --bnie  8/2/2008
						CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE
                    }

                    if (mbtype == MBTYPE_PCM)
                    {
                        status = DecodeCoefficients_PCM(1);

						//***kinoma enhancement   --bnie  8/2/2008
						CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE

                        // For PCM type MB, num coeffs are set by above call, cbp is
                        // set to all blocks coded (for deblock filter), MV are set to zero,
                        // QP is unchanged.
                        //bClearNC = false;
                        m_cur_mb.LocalMacroblockInfo->cbp4x4_luma = D_CBP_LUMA_DC | D_CBP_LUMA_AC;
                        m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[0] =
                        m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[1] = D_CBP_CHROMA_DC | D_CBP_CHROMA_AC;

                        break;  // no more to read from bitstream
                    }

                    if (IS_INTRA_MBTYPE(mbtype))
                    {
						// Get chroma prediction mode
                        m_cur_mb.LocalMacroblockInfo->intra_chroma_mode = (Ipp8u) m_pBitStream->GetVLCElement_unsigned();

 						//***kinoma enhancement   --bnie  8/2/2008
						CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE

                       if (m_cur_mb.LocalMacroblockInfo->intra_chroma_mode > 3)
                        {
                            status = UMC_BAD_STREAM;
                            break;
                        }
                    }

                    if ( (!IS_INTRA_MBTYPE(mbtype)) && (mbtype != MBTYPE_DIRECT))
					{
                        // Motion Vector Computation

                        if (bIsBSlice && (mbtype == MBTYPE_INTER_8x8 ||
                            mbtype == MBTYPE_INTER_8x8_REF0))
                        {
                            // First, if B slice and MB is 8x8, set the MV for any DIRECT
                            // 8x8 partitions. The MV for the 8x8 DIRECT partition need to
                            // be properly set before the MV for subsequent 8x8 partitions
                            // can be computed, due to prediction. The DIRECT MV are computed
                            // by a separate function and do not depend upon block neighbors for
                            // predictors, so it is done here first.
                            if (!m_bUseSpatialDirectMode)
                            {
                                // temporal DIRECT prediction
                                Ipp32u sb, sboffset;
                                sboffset = 0;
                                for (sb=0; sb<4; sb++)
                                {
                                    if (m_cur_mb.GlobalMacroblockInfo->sbtype[sb] == SBTYPE_DIRECT)
                                    {
                                        // set DIRECT motion vectors
                                        Ipp32s yM=subblock_block_mapping[sb];
                                        Ipp32s mb_col = GetColocatedLocation(pRefPicList1[0],0,yM);
                                        if (!bUseDirect8x8Inference &&
                                            IS_INTER_MBTYPE(pRefPicList1[0]->m_mbinfo.mbs[mb_col].mbtype))
                                            DecodeDirectMotionVectorsTemporal(/*pMB,*/sboffset,
                                            pRefPicList0, pRefPicList1,&pFields_stub,&pFields_stub,
                                            true);
                                        else
                                            DecodeDirectMotionVectorsTemporal_8x8Inference(
                                            //pMB,
                                            pRefPicList0, pRefPicList1,&pFields_stub,&pFields_stub,
                                            sb);
                                    }
                                    if (sb == 1)
                                        sboffset += 8 - 2;
                                    else
                                        sboffset += 2;
                                }   // for sb
                            }   // temporal DIRECT
                            else
                            {
                                if (m_cur_mb.GlobalMacroblockInfo->sbtype[0] == SBTYPE_DIRECT ||
                                    m_cur_mb.GlobalMacroblockInfo->sbtype[1] == SBTYPE_DIRECT ||
                                    m_cur_mb.GlobalMacroblockInfo->sbtype[2] == SBTYPE_DIRECT ||
                                    m_cur_mb.GlobalMacroblockInfo->sbtype[3] == SBTYPE_DIRECT)

                                    // spatial DIRECT, will set MV for any DIRECT 8x8 subblock
                                    DecodeDirectMotionVectorsSpatial(
                                    //pMB,
                                    pRefPicList0, pRefPicList1,0,
                                    bUseDirect8x8Inference);
                            }   // spatial DIRECT
                        }   // set 8x8 DIRECT in B slice

                        // MV and Ref Index

                        status = DecodeMotionVectors(/*pMB,*/ bIsBSlice);
						//***kinoma enhancement   --bnie  8/2/2008
						CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE

                        if (status != UMC_OK)
                            break;
                        bClearMV = false;
                    }

                    // cbp
                    if (mbtype != MBTYPE_INTRA_16x16)
                    {
                        m_cur_mb.LocalMacroblockInfo->cbp = DecodeCBP_CAVLC(mbtype, 1);
                        if (m_cur_mb.LocalMacroblockInfo->cbp == 255)
                        {
                            status = UMC_BAD_STREAM;
                            break;
                        }
                    }

                    if (m_cur_mb.LocalMacroblockInfo->cbp || (mbtype == MBTYPE_INTRA_16x16))
                    {
						// check for usual case of zero QP delta
                        if (!m_pBitStream->NextBit())
                        {
                            // Update QP with delta from bitstream
                            Ipp8s qpdelta = (Ipp8s) m_pBitStream->GetVLCElement_signed();
 							
							//***kinoma enhancement   --bnie  8/2/2008
							CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE

                            m_cur_mb.LocalMacroblockInfo->QP = m_cur_mb.LocalMacroblockInfo->QP  + qpdelta;
                            if (qpdelta > QP_MAX/2 || qpdelta < (-QP_MAX-1)/2)
                            {
                                /*
                                status = UMC_BAD_STREAM;
                                break;*/
                                //disabled since old h.264 files incompatibility
                            }
                        }

                        //
                        // Now, decode the coefficients
                        //
						//***kinoma enhancement   --bnie 7/11/2008
						CHECK_PACKET_LOSS_IN_NORMAL_MODE
			
                        // Get the nonzero block coefficients from the bitstream.
                        //if (cbp || (mbtype == MBTYPE_INTRA_16x16))
                        {
                            if (mbtype == MBTYPE_INTRA_16x16)
							{
                                status = DecodeCoeffs16x16BMEH_CAVLC();
							}
							else
							{
                                status = DecodeCoeffs4x4BMEH_CAVLC();
							}
                        }
                        //bClearNC = false;
						
						//***kinoma enhancement   --bnie  8/2/2008
						CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE
                    }
                    m_cur_mb.LocalMacroblockInfo->QP  = QPFromCode(m_cur_mb.LocalMacroblockInfo->QP);
                    quant_prev = m_cur_mb.LocalMacroblockInfo->QP;
                    break;
                } // while 1


            } // if MBSkipCount <= 0

            // quit if error detected
            if (status != UMC_OK)
                break;

            // Note DecodeMacroBlockType above will change MBSkipCount value
            if (MBSkipCount)
            {
                mbtype  = m_cur_mb.GlobalMacroblockInfo->mbtype = MBTYPE_SKIPPED;
                UpdateNeighbouringBlocksBMEH();//new version
                // skip MB, set up MB vars for correct decoding. If not a B slice,
                // treat MB as 16x16 INTER with no coefficients, need to compute
                // motion vector. If B slice, treat MB as DIRECT with no coefficients.
                if (!bIsBSlice)
                {
                    bClearMV = DecodeSkipMotionVectors();
                }

                MBSkipCount--;
                if (MBSkipCount == 0)
                    MBSkipCount = -1;   // signal to DecodeMacroBlockType that skip run just completed
            }
            //if (status != UMC_OK) -- Already check in above codes
            //    break;

            if (bIsBSlice && (m_cur_mb.GlobalMacroblockInfo->mbtype == MBTYPE_SKIPPED ||
                m_cur_mb.GlobalMacroblockInfo->mbtype == MBTYPE_DIRECT))
            {
                // set DIRECT motion vectors for the MB
                if (!m_bUseSpatialDirectMode)
                {
                    // temporal prediction
                    Ipp32s yM=0;
                    Ipp32s mb_col = GetColocatedLocation(pRefPicList1[0],0,yM);
                    if (!bUseDirect8x8Inference && IS_INTER_MBTYPE(pRefPicList1[0]->m_mbinfo.mbs[mb_col].mbtype))
                        DecodeDirectMotionVectorsTemporal(/*pMB,*/0, pRefPicList0, pRefPicList1,&pFields_stub,&pFields_stub,
                        false);
                    else
                        DecodeDirectMotionVectorsTemporal_8x8Inference(/*pMB,*/ pRefPicList0,
                        pRefPicList1,&pFields_stub,&pFields_stub, -1);
                }
                else
                {
                    // spatial prediction
                    DecodeDirectMotionVectorsSpatial(/*pMB,*/ pRefPicList0,
                        pRefPicList1,0,
                        bUseDirect8x8Inference);
                }
                bClearMV = false;

            }

            if (bClearMV)
            {
                //++++++++++++++++++++++++++++++++++++++++++
                // zero out stored motion vectors. 4 mvx,mvy pairs, 4 rows
                // also init ref index to -1 for all subblocks (4 bytes per row)
                // or to 0 when MB was P frame skipped.
                // pMV is 8-byte aligned for a MB
                H264DecoderMacroblockMVs *pMV = m_cur_mb.MVs[0];
                H264DecoderMacroblockRefIdxs *pRefIndex = m_cur_mb.RefIdxs[0];
                // zero out stored motion vectors. 4 mvx,mvy pairs, 4 rows
                // also init ref index to -1 for all subblocks (4 bytes per row)
                // or to 0 when MB was P frame skipped.
                // pMV is 8-byte aligned for a MB
                if ((m_cur_mb.GlobalMacroblockInfo->mbtype == MBTYPE_SKIPPED) && !bIsBSlice)
                    memset((void *)pRefIndex,0,sizeof(H264DecoderMacroblockRefIdxs));
                else
                    memset((void *)pRefIndex,-1,sizeof(H264DecoderMacroblockRefIdxs));

                memset((void *)pMV, 0, sizeof(H264DecoderMacroblockMVs));//clear all 16 vectors
                if (bIsBSlice)
                {
                    // INTRA MB in B slice
                    // zero MV in backward MV storage for all subblocks
                    pMV = m_cur_mb.MVs[1];
                    memset((void *)pMV, 0, sizeof(H264DecoderMacroblockMVs));//clear all 16 vectors
                }
            }
    // reconstruct starts here
            // Perform motion compensation to reconstruct the YUV data
            //
            offsetY = mbXOffset + (mbYOffset * uPitch);
            offsetC = offsetY >> 1;
            intra = bool(IS_INTRA_MBTYPE(mbtype));
            intra16x16 = bool(mbtype == MBTYPE_INTRA_16x16);

            QPChromaIndex = m_cur_mb.LocalMacroblockInfo->QP + ChromaQPOffset;

            QPChromaIndex = MIN(QPChromaIndex, (Ipp32s)QP_MAX);
            QPChromaIndex = MAX(0, QPChromaIndex);
            QPChroma = QPtoChromaQP[QPChromaIndex];

            Ipp32u cbp4x4_temp = CreateIPPCBPMask420(m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[0],
                                                     m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[1]);

            if (intra)
            {
                if (mbtype != MBTYPE_PCM)
                {
                    Ipp8u edge_type = 0;
#ifdef WWD_DEBUG
                    Ipp8u edge_type_2t = 0;
                    Ipp8u edge_type_2b = 0;
#endif
                    Ipp32s nLeft, nTop, nTopLeft, nTopRight;
                    Ipp32u rec_pitch = uPitch;
                    //Ipp8u special_MBAFF_case =0;

                    nLeft = m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num;
                    nTop = m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num;
                    nTopLeft = m_cur_mb.CurrentBlockNeighbours.mb_above_left.mb_num;
                    nTopRight = m_cur_mb.CurrentBlockNeighbours.mb_above_right.mb_num;
                    
					{
                        if (bUseConstrainedIntra)
                        {
                            if (0 > nLeft)
                                edge_type |= IPPVC_LEFT_EDGE;
                            else
                                if (!IS_INTRA_MBTYPE(m_pCurrentFrame->m_mbinfo.mbs[nLeft].mbtype)) edge_type |= IPPVC_LEFT_EDGE;
                            if (0 > nTop)
                                edge_type |= IPPVC_TOP_EDGE;
                            else
                                if (!IS_INTRA_MBTYPE(m_pCurrentFrame->m_mbinfo.mbs[nTop].mbtype)) edge_type |= IPPVC_TOP_EDGE;
                            if (0 > nTopLeft)
                                edge_type |= IPPVC_TOP_LEFT_EDGE;
                            else
                                if (!IS_INTRA_MBTYPE(m_pCurrentFrame->m_mbinfo.mbs[nTopLeft].mbtype)) edge_type |= IPPVC_TOP_LEFT_EDGE;
                            if (0 > nTopRight)
                                edge_type |= IPPVC_TOP_RIGHT_EDGE;
                            else
                                if (!IS_INTRA_MBTYPE(m_pCurrentFrame->m_mbinfo.mbs[nTopRight].mbtype)) edge_type |= IPPVC_TOP_RIGHT_EDGE;
                        }
                        else
                        {
                            if (0 > nLeft)
                                edge_type |= IPPVC_LEFT_EDGE;
                            if (0 > nTop)
                                edge_type |= IPPVC_TOP_EDGE;
                            if (0 > nTopLeft)
                                edge_type |= IPPVC_TOP_LEFT_EDGE;
                            if (0 > nTopRight)
                                edge_type |= IPPVC_TOP_RIGHT_EDGE;
                        }

                    }

                    // reconstruct luma block(s)
                    if (intra16x16)
                    {

                        sts = ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_x(
                            &m_pCoeffBlocksRead,
                            pYPlane + offsetY,	
                            rec_pitch,
                            (IppIntra16x16PredMode_H264) pMBIntraTypes[0],
                            m_cur_mb.LocalMacroblockInfo->cbp4x4_luma,
                            m_cur_mb.LocalMacroblockInfo->QP,
                            edge_type);

                    }
                    else
                    {
//#define WWD_DEBUG
#ifdef WWD_DEBUG
						Ipp16s *temp_pointer;
						Ipp8u  temp[16][16], temp1[16][16];
						///*** WWD_DEBUG: we do not have such bitstream to test ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_x, so ,just do it in this way
						{
							// Please read this first: -WWD in 2007-03-02
							// When we want to test ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_x, we need the following four statements
							edge_type_2t = edge_type_2b = edge_type;
							edge_type_2b &= 0xFB; // Can not be top_edge for bottom part
							edge_type_2b |= 0x20; // Always top_right edge
							(edge_type&0x01)? (edge_type_2b |= 0x10):(edge_type_2b &= 0xEF); // TOP_LEFT is LEFT of above
						}
#endif

                            sts = ippiReconstructLumaIntraMB_H264_16s8u_C1R_x(
                                &m_pCoeffBlocksRead,
                                pYPlane + offsetY,
                                rec_pitch,
                                (IppIntra4x4PredMode_H264 *) pMBIntraTypes,
                                m_cur_mb.LocalMacroblockInfo->cbp4x4_luma,
                                m_cur_mb.LocalMacroblockInfo->QP,
                                edge_type);
                    }
                    VM_ASSERT(sts >= ippStsNoErr);

#ifdef WWD_DEBUG
					{
						Ipp16s *temp_pointer;
						Ipp8u  temp[8][8], temp1[8][8];
						///*** WWD_DEBUG: we do not have such bitstream to test ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_x, so ,just do it in this way
						{
							// Please read this first: -WWD in 2007-03-02
							// Chroma is different from Luma
							// When we want to test ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_x, we need the following four statements
							edge_type_2t = edge_type_2b = edge_type;
							edge_type_2b &= 0xFB; // Can not be top_edge for bottom part
							edge_type_2b |= 0x20; // Always top_right edge
							(edge_type&0x01)? (edge_type_2b |= 0x10):(edge_type_2b &= 0xEF); // TOP_LEFT is LEFT of above
						}
						temp_pointer = m_pCoeffBlocksRead;
					}
#endif

                    // reconstruct chroma block(s)

                        sts = ippiReconstructChromaIntraMB_H264_16s8u_P2R_x (
                            &m_pCoeffBlocksRead,
                            pUPlane + offsetC,
                            pVPlane + offsetC,
                            rec_pitch,
                            (IppIntraChromaPredMode_H264) m_cur_mb.LocalMacroblockInfo->intra_chroma_mode,
                            (Ipp32u) cbp4x4_temp,
                            QPChroma,
                            edge_type);
                    VM_ASSERT(sts >= ippStsNoErr);
                }
                else
                {
                    // reconstruct PCM block(s)
                    ReconstructPCMMB(offsetY,offsetC, 1);
                }
            }
            else
            {			
				/*
                Ipp32s RefF_pic_cnt = 0, RefB_pic_cnt = 0;*/

                //Ipp8s *pRefIndex = m_cur_mb.RefIdxs[0]->RefIdxs;
                //Ipp8s *pRefIndexBack = m_cur_mb.RefIdxs[1]->RefIdxs;
                {
                    ReconstructMacroblockBMEHC(pYPlane + offsetY,
                        pVPlane + offsetC,
                        pUPlane + offsetC,
                        mbXOffset,
                        mbYOffset,
                        bUseDirect8x8Inference,
                        pRefPicList0,
                        pRefPicList1);
                }


                if (m_cur_mb.LocalMacroblockInfo->cbp4x4_luma & D_CBP_LUMA_AC)
                {

                    sts = ippiReconstructLumaInterMB_H264_16s8u_C1R_x(&m_pCoeffBlocksRead,
                                                                    pYPlane + offsetY,
                                                                    uPitch << pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo),
                                                                    m_cur_mb.LocalMacroblockInfo->cbp4x4_luma,
                                                                    m_cur_mb.LocalMacroblockInfo->QP);
                    VM_ASSERT(sts >= ippStsNoErr);
                }

                if (m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[0]
                    || m_cur_mb.LocalMacroblockInfo->cbp4x4_chroma[1])
                {
                    sts = ippiReconstructChromaInterMB_H264_16s8u_P2R_x(&m_pCoeffBlocksRead,
                        pUPlane + offsetC,
                        pVPlane + offsetC,
                        uPitch << pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo),
                        (Ipp32u) cbp4x4_temp,
                        QPChroma);
                    VM_ASSERT(sts >= ippStsNoErr);
                }
            }

            if (UMC_OK != status)
                break;

            // Next MB
            m_CurMBAddr = NextMB;
        }   // for nCurMBNumber, VLC loop
    }

//***kinoma enhancement
packet_loss_happened:

    if ((UMC_OK != status) &&
        (UMC_END_OF_STREAM != status))
        return status;

    *pnCurMBNumber = nCurMBNumber;/*
    m_pSliceStream->m_nQuantPrev = quant_prev;
    m_pSliceStream->m_nMBSkipCount = MBSkipCount;
    m_pSliceStream->m_nPassFDFDecode = PassFDFDecode;*/
    m_pSlice->SetStateVariables(MBSkipCount, quant_prev, PassFDFDecode);

    return status;

} // Status H264SegmentDecoder::DecodeSegmentCAVLC(void)

} // namespace UMC
