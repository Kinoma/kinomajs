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

/*  DEBUG: this file is requred to changing name
    to umc_h264_dec_slice_decode_pic */

//***
#include "kinoma_ipp_lib.h"

#include "umc_h264_slice_decoding.h"
#include "umc_h264_dec_slice_decoder.h"
#include "umc_h264_dec.h"
#include "vm_debug.h"


namespace UMC
{

struct H264RefListInfo
{
    Ipp32s m_iNumShortEntriesInList;
    Ipp32s m_iNumLongEntriesInList;
    Ipp32s m_iNumFramesInL0List;
    Ipp32s m_iNumFramesInL1List;
    Ipp32s m_iNumFramesInLTList;
};

Status H264Slice::UpdateRefPicList(H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList)
{
    Status ps = UMC_OK;
    RefPicListReorderInfo *pReorderInfo_L0 = &m_ReorderInfoL0;
    RefPicListReorderInfo *pReorderInfo_L1 = &m_ReorderInfoL1;
    H264SeqParamSet *sps = GetSeqParam();
    Ipp32u uMaxFrameNum;
    Ipp32u uMaxPicNum;
    H264VideoDecoder::H264DecoderFrame *pFrm;
    H264VideoDecoder::H264DecoderFrame *pHead = pDecoderFrameList->head();
    //Ipp32u i;
    H264VideoDecoder::H264DecoderFrame **pRefPicList0;
    H264VideoDecoder::H264DecoderFrame **pRefPicList1;
    //***bnie: Ipp8s *pFields0;
    //***bnie: Ipp8s *pFields1;
    Ipp32u NumShortTermRefs, NumLongTermRefs;
    H264RefListInfo rli;

    VM_ASSERT(m_pCurrentFrame);

	//***kinoma enhancement   --bnie 8/2/2008
	pRefPicList0 = m_pCurrentFrame->GetRefPicListSafe( m_iNumber, 0 );
	pRefPicList1 = m_pCurrentFrame->GetRefPicListSafe( m_iNumber, 1 );
	//***bnie: pFields0     = m_pCurrentFrame->GetPedictionSafe(  m_iNumber, 0 );	
	//***bnie: pFields1     = m_pCurrentFrame->GetPedictionSafe(  m_iNumber, 1 );	

    //pRefPicList0 = m_pCurrentFrame->GetRefPicList(m_iNumber, 0)->m_RefPicList;
    //pRefPicList1 = m_pCurrentFrame->GetRefPicList(m_iNumber, 1)->m_RefPicList;
    //pFields0     = m_pCurrentFrame->GetRefPicList(m_iNumber, 0)->m_Prediction;
    //pFields1     = m_pCurrentFrame->GetRefPicList(m_iNumber, 1)->m_Prediction;

    // Spec reference: 8.2.4, "Decoding process for reference picture lists
    // construction"

    // get pointers to the picture and sequence parameter sets currently in use
    uMaxFrameNum = (1<<sps->log2_max_frame_num);
    uMaxPicNum = uMaxFrameNum;//***bnie: (m_SliceHeader.field_pic_flag == 0) ? uMaxFrameNum : uMaxFrameNum<<1;
/*    if(m_field_index && m_pCurrentFrame->m_PictureStructureForRef<FRM_STRUCTURE)
    {
        if (m_NALRefIDC[0])
        {
            m_pCurrentFrame->SetisShortTermRef(0);
        }
        // If B slice, init scaling factors array
    }*/

    for (pFrm = pHead; pFrm; pFrm = pFrm->future())
    {
        // update FrameNumWrap and PicNum if frame number wrap occurred,
        // for short-term frames
        // TBD: modify for fields
        pFrm->UpdateFrameNumWrap((Ipp32s)m_SliceHeader.frame_num,
            uMaxFrameNum,
            m_pCurrentFrame->m_PictureStructureForRef
			//***bnie: +m_pCurrentFrame->m_bottom_field_flag[0]
			);

        // For long-term references, update LongTermPicNum. Note this
        // could be done when LongTermFrameIdx is set, but this would
        // only work for frames, not fields.
        // TBD: modify for fields
        pFrm->UpdateLongTermPicNum(m_pCurrentFrame->m_PictureStructureForRef
									//***bnie: + m_pCurrentFrame->m_bottom_field_flag[0]
									);
    }

    if ((m_SliceHeader.slice_type != INTRASLICE) && (m_SliceHeader.slice_type != S_INTRASLICE))
    {
        // Detect and report no available reference frames
        pDecoderFrameList->countActiveRefs(NumShortTermRefs, NumLongTermRefs);
        if ((NumShortTermRefs + NumLongTermRefs) == 0)
        {
            VM_ASSERT(0);
            ps = UMC_BAD_STREAM;
        }

        if (ps == UMC_OK)
        {
            // Initialize the reference picture lists
            // Note the slice header get function always fills num_ref_idx_lx_active
            // fields with a valid value; either the override from the slice
            // header in the bitstream or the values from the pic param set when
            // there is no override.
            if ((m_SliceHeader.slice_type == PREDSLICE) || (m_SliceHeader.slice_type == S_PREDSLICE))
                InitPSliceRefPicList(m_SliceHeader.num_ref_idx_l0_active,pRefPicList0,pDecoderFrameList);
            else
            {
                //pRefPicList1 = m_pCurrentFrame->GetRefPicList(m_iNumber, 1)->m_RefPicList;
                InitBSliceRefPicLists(  m_SliceHeader.num_ref_idx_l0_active,m_SliceHeader.num_ref_idx_l1_active,
                                        pRefPicList0, pRefPicList1,pDecoderFrameList,rli);
            }


            // Reorder the reference picture lists
            //Ipp8u num_ST_Ref0 = 0, num_LT_Ref = 0;

            //***bnie: if (BPREDSLICE == m_SliceHeader.slice_type)
            //***bnie: {
            //***bnie: }
            if (pReorderInfo_L0->num_entries > 0)
                ReOrderRefPicList(pRefPicList0, pReorderInfo_L0, uMaxPicNum, m_SliceHeader.num_ref_idx_l0_active);
            if (BPREDSLICE == m_SliceHeader.slice_type)
            {
                if (pReorderInfo_L1->num_entries > 0)
                   ReOrderRefPicList(pRefPicList1, pReorderInfo_L1, uMaxPicNum, m_SliceHeader.num_ref_idx_l1_active);
            }
            // If B slice, init scaling factors array
            if ((BPREDSLICE == m_SliceHeader.slice_type) && (pRefPicList1[0] != NULL))
                InitDistScaleFactor(m_SliceHeader.num_ref_idx_l0_active,
                        pRefPicList0, pRefPicList1);

        }

		//if(0)
		{
			//***dealing with packet loss, create makeshift reflist 0,1
			if ((m_SliceHeader.slice_type == PREDSLICE) || (m_SliceHeader.slice_type == S_PREDSLICE))
			{
				if( pRefPicList0[0] == NULL )
				{
                	InitPSliceRefPicList_default(m_SliceHeader.num_ref_idx_l0_active,pRefPicList0,pDecoderFrameList);
				}
			}
			else
			{
				if( pRefPicList0[0] == NULL ||  pRefPicList1[0] == NULL )
				{
	                InitBSliceRefPicLists_default(pRefPicList0[0] == NULL,pRefPicList1[0] == NULL,
												pRefPicList0, pRefPicList1, pDecoderFrameList,rli);
				}

			}

            // Reorder the reference picture lists
            //Ipp8u num_ST_Ref0 = 0, num_LT_Ref = 0;

            if (pReorderInfo_L0->num_entries > 0)
                ReOrderRefPicList(pRefPicList0, pReorderInfo_L0, uMaxPicNum, m_SliceHeader.num_ref_idx_l0_active);
            if (BPREDSLICE == m_SliceHeader.slice_type)
            {
                if (pReorderInfo_L1->num_entries > 0)
					ReOrderRefPicList(pRefPicList1, pReorderInfo_L1, uMaxPicNum, m_SliceHeader.num_ref_idx_l1_active);
            }
            // If B slice, init scaling factors array
            if ((BPREDSLICE == m_SliceHeader.slice_type) && (pRefPicList1[0] != NULL))
                InitDistScaleFactor(m_SliceHeader.num_ref_idx_l0_active,
                        pRefPicList0, pRefPicList1);

		}
        
    }

    m_bNeedToUpdateRefPicList = false;

    return ps;

} // Status H264Slice::UpdateRefPicList(H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList)

void H264Slice::InitPSliceRefPicList(Ipp32s /*NumL0RefActive*/,
                                     H264VideoDecoder::H264DecoderFrame **pRefPicList,
                                     H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList)
{
    Ipp32s i, j, k;
    Ipp32s NumFramesInList;
    H264VideoDecoder::H264DecoderFrame *pHead = pDecoderFrameList->head();
    H264VideoDecoder::H264DecoderFrame *pFrm;
    Ipp32s PicNum;
    bool bError = false;

    VM_ASSERT(pRefPicList);

    for (i=0; i<MAX_NUM_REF_FRAMES; i++)
    {
        pRefPicList[i] = NULL;
    }
    NumFramesInList = 0;

    //***bnie: if (!bIsFieldSlice)
    {
        // Frame. Ref pic list ordering: Short term largest pic num to
        // smallest, followed by long term, largest long term pic num to
        // smallest. Note that ref pic list has one extra slot to assist
        // with re-ordering.
        for (pFrm = pHead; pFrm; pFrm = pFrm->future())
        {
            if (pFrm->isShortTermRef()==3)
            {
                // add to ordered list
                PicNum = pFrm->PicNum(0);

                // find insertion point
                j=0;
                while (j<NumFramesInList &&
                        pRefPicList[j]->isShortTermRef() &&
                        pRefPicList[j]->PicNum(0) > PicNum)
                    j++;

                // make room if needed
                if (pRefPicList[j])
                {
                    for (k=NumFramesInList; k>j; k--)
                    {
                        // Avoid writing beyond end of list
                        if (k > MAX_NUM_REF_FRAMES-1)
                        {
                            VM_ASSERT(0);
                            bError = true;
                            break;
                        }
                        pRefPicList[k] = pRefPicList[k-1];
                    }
                }

                // add the short-term reference
                pRefPicList[j] = pFrm;
                NumFramesInList++;
            }
            else if (pFrm->isLongTermRef()==3)
            {
                // add to ordered list
                PicNum = pFrm->LongTermPicNum(0,3);

                // find insertion point
                j=0;
                // Skip past short-term refs and long term refs with smaller
                // long term pic num
                while (j<NumFramesInList &&
                        (pRefPicList[j]->isShortTermRef() ||
                        (pRefPicList[j]->isLongTermRef() &&
                        pRefPicList[j]->LongTermPicNum(0,2) < PicNum)))
                    j++;

                // make room if needed
                if (pRefPicList[j])
                {
                    for (k=NumFramesInList; k>j; k--)
                    {
                        // Avoid writing beyond end of list
                        if (k > MAX_NUM_REF_FRAMES-1)
                        {
                            VM_ASSERT(0);
                            bError = true;
                            break;
                        }
                        pRefPicList[k] = pRefPicList[k-1];
                    }
                }

                // add the long-term reference
                pRefPicList[j] = pFrm;
                NumFramesInList++;
            }
            if (bError) break;
        }

		//***bnie: to be robust	in case we run into referencing NULL pointer 		3/20/2009
		for (i=1; i<MAX_NUM_REF_FRAMES; i++)
			if( pRefPicList[i] == NULL )
				pRefPicList[i] = (H264VideoDecoder::H264DecoderFrame *)pRefPicList[i-1];

    }

    // If the number of reference pictures on the L0 list is greater than the
    // number of active references, discard the "extras".
    //I realy don't know why...
    /*if (NumFramesInList > NumL0RefActive)
    {
        for (i=NumFramesInList-1; i>=NumL0RefActive; i--)
            pRefPicList[i] = NULL;
    }*/

} // void H264Slice::InitPSliceRefPicList(bool bIsFieldSlice,

void H264Slice::InitBSliceRefPicLists(Ipp32s /*NumL0RefActive*/,
                                      Ipp32s /*NumL1RefActive*/,
                                      H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                      H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                      H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList,
                                      H264RefListInfo &rli)
{
    Ipp32s i, j, k;
    Ipp32s NumFramesInL0List;
    Ipp32s NumFramesInL1List;
    Ipp32s NumFramesInLTList;
    H264VideoDecoder::H264DecoderFrame *pHead = pDecoderFrameList->head();
    H264VideoDecoder::H264DecoderFrame *pFrm;
    Ipp32s FrmPicOrderCnt;
    H264VideoDecoder::H264DecoderFrame *LTRefPicList[MAX_NUM_REF_FRAMES];    // temp storage for long-term ordered list
    Ipp32s LongTermPicNum;
    bool bError = false;

    for (i=0; i<MAX_NUM_REF_FRAMES; i++)
    {
        pRefPicList0[i] = NULL;
        pRefPicList1[i] = NULL;
        LTRefPicList[i] = NULL;
    }
    NumFramesInL0List = 0;
    NumFramesInL1List = 0;
    NumFramesInLTList = 0;

    //***bnie: if (!bIsFieldSlice)
    {
        Ipp32s CurrPicOrderCnt = m_pCurrentFrame->PicOrderCnt_kinoma_always_frame();
        // Short term references:
        // Need L0 and L1 lists. Both contain 2 sets of reference frames ordered
        // by PicOrderCnt. The "previous" set contains the reference frames with
        // a PicOrderCnt < current frame. The "future" set contains the reference
        // frames with a PicOrderCnt > current frame. In both cases the ordering
        // is from closest to current frame to farthest. L0 has the previous set
        // followed by the future set; L1 has the future set followed by the previous set.
        // Accomplish this by one pass through the decoded frames list creating
        // the ordered previous list in the L0 array and the ordered future list
        // in the L1 array. Then copy from both to the other for the second set.

        // Long term references:
        // The ordered list is the same for L0 and L1, is ordered by ascending
        // LongTermPicNum. The ordered list is created using local temp then
        // appended to the L0 and L1 lists after the short term references.

        for (pFrm = pHead; pFrm; pFrm = pFrm->future())
        {
            if (pFrm->isShortTermRef()==3)
            {
                // add to ordered list
                FrmPicOrderCnt = pFrm->PicOrderCnt_kinoma_always_frame();

                if (FrmPicOrderCnt < CurrPicOrderCnt)
                {
                    // Previous reference to L0, order large to small
                    j=0;
                    while (j<NumFramesInL0List &&
                            (pRefPicList0[j]->PicOrderCnt_kinoma_always_frame() > FrmPicOrderCnt))
                        j++;

                    // make room if needed
                    if (pRefPicList0[j])
                    {
                        for (k=NumFramesInL0List; k>j; k--)
                        {
                            // Avoid writing beyond end of list
                            if (k > MAX_NUM_REF_FRAMES-1)
                            {
                                VM_ASSERT(0);
                                bError = true;
                                break;
                            }
                            pRefPicList0[k] = pRefPicList0[k-1];
                        }
                    }

                    // add the short-term reference
                    pRefPicList0[j] = pFrm;
                    NumFramesInL0List++;
                }
                else
                {
                    // Future reference to L1, order small to large
                    j=0;
                    while (j<NumFramesInL1List &&
                            pRefPicList1[j]->PicOrderCnt_kinoma_always_frame() < FrmPicOrderCnt)
                        j++;

                    // make room if needed
                    if (pRefPicList1[j])
                    {
                        for (k=NumFramesInL1List; k>j; k--)
                        {
                            // Avoid writing beyond end of list
                            if (k > MAX_NUM_REF_FRAMES-1)
                            {
                                VM_ASSERT(0);
                                bError = true;
                                break;
                            }
                            pRefPicList1[k] = pRefPicList1[k-1];
                        }
                    }

                    // add the short-term reference
                    pRefPicList1[j] = pFrm;
                    NumFramesInL1List++;
                }
            }    // short-term B
            else if (pFrm->isLongTermRef()==3)
            {
                // long term reference
                LongTermPicNum = pFrm->LongTermPicNum(0,3);

                // order smallest to largest
                j=0;
                while (j<NumFramesInLTList &&
                        LTRefPicList[j]->LongTermPicNum(0) < LongTermPicNum)
                    j++;

                // make room if needed
                if (LTRefPicList[j])
                {
                    for (k=NumFramesInLTList; k>j; k--)
                    {
                        // Avoid writing beyond end of list
                        if (k > MAX_NUM_REF_FRAMES-1)
                        {
                            VM_ASSERT(0);
                            bError = true;
                            break;
                        }
                        LTRefPicList[k] = LTRefPicList[k-1];
                    }
                }

                // add the long-term reference
                LTRefPicList[j] = pFrm;
                NumFramesInLTList++;

            }    // long term reference

            if (bError) break;

        }    // for pFrm
        if ((NumFramesInL0List+NumFramesInL1List+NumFramesInLTList) < MAX_NUM_REF_FRAMES)
        {
            // Complete L0 and L1 lists
            // Add future short term references to L0 list, after previous
            for (i=0; i<NumFramesInL1List; i++)
                pRefPicList0[NumFramesInL0List+i] = pRefPicList1[i];


            // Add previous short term references to L1 list, after future
            for (i=0; i<NumFramesInL0List; i++)
                pRefPicList1[NumFramesInL1List+i] = pRefPicList0[i];


            // Add long term list to both L0 and L1
            for (i=0; i<NumFramesInLTList; i++)
            {
                pRefPicList0[NumFramesInL0List+NumFramesInL1List+i] = LTRefPicList[i];
                pRefPicList1[NumFramesInL0List+NumFramesInL1List+i] = LTRefPicList[i];
            }

            // Special rule: When L1 has more than one entry and L0 == L1, all entries,
            // swap the first two entries of L1.
            // They can be equal only if there are no future or no previous short term
            // references.
            if ((NumFramesInL0List == 0 || NumFramesInL1List == 0) &&
                ((NumFramesInL0List+NumFramesInL1List+NumFramesInLTList) > 1))
            {
                pRefPicList1[0] = pRefPicList0[1];
                pRefPicList1[1] = pRefPicList0[0];
            }
        }
        else
        {
            // too many reference frames
            VM_ASSERT(0);
        }

    }    // not field slice



    // If the number of reference pictures on the lists is greater than the
    // number of active references, discard the "extras".
    /*
    if (NumFramesInL0List > NumL0RefActive)
    {
        for (i=NumFramesInL0List-1; i>=NumL0RefActive; i--)
            pRefPicList0[i] = NULL;
    }
    if (NumFramesInL1List > NumL1RefActive)
    {
        for (i=NumFramesInL1List-1; i>=NumL1RefActive; i--)
            pRefPicList1[i] = NULL;
    }*/
    rli.m_iNumFramesInL0List = NumFramesInL0List;
    rli.m_iNumFramesInL1List = NumFramesInL1List;
    rli.m_iNumFramesInLTList = NumFramesInLTList;

} // void H264Slice::InitBSliceRefPicLists(bool bIsFieldSlice,


void VisibleFirst( H264VideoDecoder::H264DecoderFrame **ref, int n )
{
	H264VideoDecoder::H264DecoderFrame *f = NULL;
	int i;

	for( i = 0; i < n; i++ )
	{
		f = ref[i];
		if( f->isDisplayable() )
			break;
	}

	if( f != NULL )
	{
		H264VideoDecoder::H264DecoderFrame *t;

		t = ref[0];
		ref[0] = f;
		ref[i] = t;
	}
}


void H264Slice::InitPSliceRefPicList_default(Ipp32s /*NumL0RefActive*/,
                                     H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                     H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList)
{
    Ipp32s i, j;
    Ipp32s NumFramesInL0List;
    //Ipp32s NumFramesInL1List;
    Ipp32s NumFramesInLTList;
    H264VideoDecoder::H264DecoderFrame *pHead = pDecoderFrameList->head();
    H264VideoDecoder::H264DecoderFrame *pFrm;
    //Ipp32s FrmPicOrderCnt;
    H264VideoDecoder::H264DecoderFrame *LTRefPicList[MAX_NUM_REF_FRAMES];    // temp storage for long-term ordered list
    //Ipp32s LongTermPicNum;
    //bool bError = false;

    for (i=0; i<MAX_NUM_REF_FRAMES; i++)
    {
        pRefPicList0[i] = NULL;
        LTRefPicList[i] = NULL;
    }
    NumFramesInL0List = 0;
    NumFramesInLTList = 0;

    {
		//Ipp32s CurrPicOrderCnt = m_pCurrentFrame->PicOrderCnt_kinoma_always_frame();
		j = 0;

        for (pFrm = pHead; pFrm; pFrm = pFrm->future())
        {
			// add the short-term reference
			pRefPicList0[j] = pFrm;
			NumFramesInL0List++;
            // add the long-term reference
            LTRefPicList[j] = pFrm;
            NumFramesInLTList++;

			j++;
			if( j >= MAX_NUM_REF_FRAMES )
				break;
		}

		VisibleFirst( pRefPicList0, j );

    }    // not field slice
} // void H264Slice::InitPSliceRefPicList(bool bIsFieldSlice,

void H264Slice::InitBSliceRefPicLists_default(Ipp32s emptyL0,
                                      Ipp32s emptyL1,
                                      H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                      H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                      H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList,
                                      H264RefListInfo &rli)
{
    Ipp32s j;
    Ipp32s NumFramesInL0List;
    Ipp32s NumFramesInL1List;
    Ipp32s NumFramesInLTList;
    H264VideoDecoder::H264DecoderFrame *pHead = pDecoderFrameList->head();
    H264VideoDecoder::H264DecoderFrame *pFrm;
    //Ipp32s FrmPicOrderCnt;
    H264VideoDecoder::H264DecoderFrame *LTRefPicList[MAX_NUM_REF_FRAMES];    // temp storage for long-term ordered list
    //Ipp32s LongTermPicNum;
    //bool bError = false;

    NumFramesInL0List = 0;
    NumFramesInL1List = 0;
    NumFramesInLTList = 0;

	if( emptyL0 )
    {
		//Ipp32s CurrPicOrderCnt = m_pCurrentFrame->PicOrderCnt_kinoma_always_frame();
		j = 0;

        for (pFrm = pHead; pFrm; pFrm = pFrm->future())
        {
			
			// add the short-term reference
			pRefPicList0[j] = pFrm;
			NumFramesInL0List++;
            // add the long-term reference
            LTRefPicList[j] = pFrm;
            NumFramesInLTList++;

			j++;
			if( j >= MAX_NUM_REF_FRAMES )
				break;
		}
		
		VisibleFirst( pRefPicList0, j );

		rli.m_iNumFramesInL0List = NumFramesInL0List;
		rli.m_iNumFramesInLTList = NumFramesInLTList;
    }

	if( emptyL1 )
    {
		//Ipp32s CurrPicOrderCnt = m_pCurrentFrame->PicOrderCnt_kinoma_always_frame();
		j = 0;

        for (pFrm = pHead; pFrm; pFrm = pFrm->future())
        {
			// add the short-term reference
			pRefPicList1[j] = pFrm;
			NumFramesInL1List++;
            // add the long-term reference
            LTRefPicList[j] = pFrm;
            NumFramesInLTList++;

			j++;
			if( j >= MAX_NUM_REF_FRAMES )
				break;
		}
		
		VisibleFirst( pRefPicList1, j );

		rli.m_iNumFramesInL1List = NumFramesInL1List;
		rli.m_iNumFramesInLTList = NumFramesInLTList;
    }
} // void H264Slice::InitBSliceRefPicLists(bool bIsFieldSlice,



#define CalculateDSF(index)                                                     \
        /* compute scaling ratio for temporal direct and implicit weighting*/   \
        tb = picCntCur - picCntRef0;    /* distance from previous */            \
        td = picCntRef1 - picCntRef0;    /* distance between ref0 and ref1 */   \
                                                                                \
        /* special rule: if td is 0 or if L0 is long-term reference, use */     \
        /* L0 motion vectors and equal weighting.*/                             \
        if (td == 0 || pRefPicList0[index]->isLongTermRef_kinoma_always_frame()) \
        {                                                                       \
            /* These values can be used directly in scaling calculations */     \
            /* to get back L0 or can use conditional test to choose L0.    */   \
            pDistScaleFactor[L0Index] = 128;    /* for equal weighting    */    \
            pDistScaleFactorMV[L0Index] = 256;                                  \
        }                                                                       \
        else                                                                    \
        {                                                                       \
                                                                                \
            tb = MAX(-128,tb);                                                  \
            tb = MIN(127,tb);                                                   \
            td = MAX(-128,td);                                                  \
            td = MIN(127,td);                                                   \
                                                                                \
            VM_ASSERT(td != 0);                                                    \
                                                                                \
            tx = (16384 + abs(td/2))/td;                                        \
                                                                                \
            DistScaleFactor = (tb*tx + 32)>>6;                                  \
            DistScaleFactor = MAX(-1024, DistScaleFactor);                      \
            DistScaleFactor = MIN(1023, DistScaleFactor);                       \
                                                                                \
            if (DistScaleFactor < -256 || DistScaleFactor > 512)                \
                pDistScaleFactor[L0Index] = 128;    /* equal weighting     */   \
            else                                                                \
                pDistScaleFactor[L0Index] = DistScaleFactor;                    \
                                                                                \
            pDistScaleFactorMV[L0Index] = DistScaleFactor;                      \
        }

void H264Slice::InitDistScaleFactor(Ipp32s NumL0RefActive,
                                    H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                    H264VideoDecoder::H264DecoderFrame **pRefPicList1)

{
    Ipp8s L0Index;
    Ipp32u picCntRef0;
    Ipp32u picCntRef1;
    Ipp32u picCntCur;
    Ipp32s DistScaleFactor;
    Ipp32s *pDistScaleFactor;
    Ipp32s *pDistScaleFactorMV;

    Ipp32s tb;
    Ipp32s td;
    Ipp32s tx;

    VM_ASSERT(NumL0RefActive <= MAX_NUM_REF_FRAMES);
    VM_ASSERT(pRefPicList1[0]);
    pDistScaleFactor = m_DistScaleFactor;        //frames or fields
    pDistScaleFactorMV = m_DistScaleFactorMV;  //frames or fields

	//***bnie: if(m_pCurrentFrame->m_PictureStructureForRef>=FRM_STRUCTURE )
    {
        picCntRef1 = pRefPicList1[0]->PicOrderCnt_kinoma_always_frame();
    }

	picCntCur = m_pCurrentFrame->PicOrderCnt_kinoma_always_frame();//m_PicOrderCnt;

    for (L0Index=0; L0Index<NumL0RefActive; L0Index++)
    {

		//***bnie: if(m_pCurrentFrame->m_PictureStructureForRef>=FRM_STRUCTURE )
        {
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt_kinoma_always_frame();
        }
		
		CalculateDSF(L0Index);
    }

} // void H264Slice::InitDistScaleFactor(Ipp32s NumL0RefActive,


void H264Slice::ReOrderRefPicList(H264VideoDecoder::H264DecoderFrame **pRefPicList,
                                  RefPicListReorderInfo *pReorderInfo,
                                  Ipp32s MaxPicNum,
                                  Ipp32s NumRefActive)
{
    Ipp32u i;
    Ipp32s j;
    Ipp32s PicNumNoWrap;
    Ipp32s PicNum;
    Ipp32s PicNumPred;
    Ipp32s PicNumCurr;
    H264VideoDecoder::H264DecoderFrame *tempFrame[2];
    //Ipp8s tempFields[2];
    Ipp32u NumDuplicates;

    // Reference: Reordering process for reference picture lists, 8.2.4.3
    //***bnie: if (!bIsFieldSlice)
    {
        PicNumCurr = m_pCurrentFrame->PicNum(0,3);
        PicNumPred = PicNumCurr;

        for (i=0; i<pReorderInfo->num_entries; i++)
        {
            if (pReorderInfo->reordering_of_pic_nums_idc[i] < 2)
            {
                // short term reorder
                if (pReorderInfo->reordering_of_pic_nums_idc[i] == 0)
                {
                    PicNumNoWrap = PicNumPred - pReorderInfo->reorder_value[i];
                    if (PicNumNoWrap < 0)
                        PicNumNoWrap += MaxPicNum;
                }
                else
                {
                    PicNumNoWrap = PicNumPred + pReorderInfo->reorder_value[i];
                    if (PicNumNoWrap >= MaxPicNum)
                        PicNumNoWrap -= MaxPicNum;
                }
                PicNumPred = PicNumNoWrap;

                PicNum = PicNumNoWrap;
                if (PicNum > PicNumCurr)
                    PicNum -= MaxPicNum;

                // Find the PicNum frame.
                for (j=0; pRefPicList[j] !=NULL; j++)
                    if (pRefPicList[j] != NULL &&
                        pRefPicList[j]->isShortTermRef() &&
                        pRefPicList[j]->PicNum(0,3) == PicNum)
                        break;

                // error if not found, should not happen
                VM_ASSERT(pRefPicList[j]);

                // Place picture with PicNum on list, shifting pictures
                // down by one while removing any duplication of picture with PicNum.
                tempFrame[0] = pRefPicList[j];    // PicNum frame just found
                NumDuplicates = 0;
                for (j=i; j<NumRefActive || pRefPicList[j] !=NULL; j++)
                {
                    if (NumDuplicates == 0)
                    {
                        // shifting pictures down
                        tempFrame[1] = pRefPicList[j];
                        pRefPicList[j] = tempFrame[0];
                        tempFrame[0] = tempFrame[1];
                    }
                    else if (NumDuplicates == 1)
                    {
                        // one duplicate of PicNum made room for new entry, just
                        // look for more duplicates to eliminate
                        tempFrame[0] = pRefPicList[j];
                    }
                    else
                    {
                        // >1 duplicate found, shifting pictures up
                        pRefPicList[j - NumDuplicates + 1] = tempFrame[0];
                        tempFrame[0] = pRefPicList[j];
                    }
                    if (tempFrame[0] == NULL)
                        break;        // end of valid reference frames
                    if (tempFrame[0]->isShortTermRef() &&
                        tempFrame[0]->PicNum(0,3) == PicNum)
                        NumDuplicates++;
                }
            }    // short term reorder
            else
            {
                // long term reorder
                PicNum = pReorderInfo->reorder_value[i];

                // Find the PicNum frame.
                for (j=0; pRefPicList[j] !=NULL; j++)
                    if (pRefPicList[j] != NULL &&
                        pRefPicList[j]->isLongTermRef() &&
                        pRefPicList[j]->LongTermPicNum(0,3) == PicNum)
                        break;

                // error if not found, should not happen
                VM_ASSERT(pRefPicList[j]);

                // Place picture with PicNum on list, shifting pictures
                // down by one while removing any duplication of picture with PicNum.
                tempFrame[0] = pRefPicList[j];    // PicNum frame just found
                NumDuplicates = 0;
                for (j=i; j<NumRefActive || pRefPicList[j] !=NULL; j++)
                {
                    if (NumDuplicates == 0)
                    {
                        // shifting pictures down
                        tempFrame[1] = pRefPicList[j];
                        pRefPicList[j] = tempFrame[0];
                        tempFrame[0] = tempFrame[1];
                    }
                    else if (NumDuplicates == 1)
                    {
                        // one duplicate of PicNum made room for new entry, just
                        // look for more duplicates to eliminate
                        tempFrame[0] = pRefPicList[j];
                    }
                    else
                    {
                        // >1 duplicate found, shifting pictures up
                        pRefPicList[j - NumDuplicates + 1] = tempFrame[0];
                        tempFrame[0] = pRefPicList[j];
                    }
                    if (tempFrame[0] == NULL)
                        break;        // end of valid reference frames
                    if (tempFrame[0]->isLongTermRef() &&
                        tempFrame[0]->LongTermPicNum(0,3) == PicNum)
                        NumDuplicates++;
                }
            }    // long term reorder
        }    // for i
    }

} // void H264Slice::ReOrderRefPicList(bool bIsFieldSlice,

} // namespace UMC
