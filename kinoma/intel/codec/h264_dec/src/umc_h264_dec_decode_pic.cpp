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


#include <ippi.h>
#include "umc_h264_dec.h"
#include "umc_h264_dec_slice_decoder.h"
#include "umc_h264_dec_slice_decoder_mt.h"
#include "umc_h264_dec_conversion.h"
#include "umc_video_data.h"
#include "umc_media_data_ex.h"

//***
#include "avcC.h"

#include "vm_debug.h"
#include "vm_sys_info.h"
#include "vm_thread.h"

#include "umc_h264_bitstream.h"

namespace UMC
{

enum
{
    MINIMAL_DATA_SIZE           = 4
};

// when following define is remarked SLICE_COMBINED_MODE is used
//#define MULTI_SLICE_MODE

H264VideoDecoder::H264DecoderFrame::H264DecoderFrame()
    : m_pPreviousFrame(0)
    , m_pFutureFrame(0)
    , m_cumulativeTR(0.0)
    , m_isDisplayable(false)
    , m_wasOutputted(false)
    , m_wasDisplayed(false)
    , m_lockedForDisplay(false)
//    , m_FrameNum((unsigned)-1)
    , m_bIsIDRPic(false)
    , m_pRefPicList(0)
    , m_pParsedFrameData(0)
    , m_paddedParsedFrameDataSize(sDimensions(0,0))
    /*, m_num_slice_start(0)*/
//***    , m_dFrameTime(-1.0)
{
    m_pParsedFrameDataNew = NULL;
    m_isShortTermRef[0] = m_isShortTermRef[1] = false;
    m_isLongTermRef[0] = m_isLongTermRef[1] = false;
    m_FrameNumWrap = m_FrameNum  = -1;
    m_LongTermFrameIdx = -1;
    m_RefPicListResetCount[0] = m_RefPicListResetCount[1] = 0;
    m_PicNum[0] = m_PicNum[1] = -1;
    m_LongTermPicNum[0] = m_PicNum[1] = -1;
    m_PicOrderCnt[0] = m_PicOrderCnt[1] = 0x7fffffff;

	m_PictureStructureForRef = 0;
	totalMBs = 0;
	m_crop_left = m_crop_right = m_crop_top = m_crop_bottom = 0;
	m_crop_flag = 0;
	m_mbinfo.MV[0] = m_mbinfo.MV[1] = NULL;
	m_mbinfo.RefIdxs[0] = m_mbinfo.RefIdxs[1] = NULL;
	m_mbinfo.mbs = NULL;
	m_PQUANT = 0;
    SetBusy(false);
}

void
H264VideoDecoder::H264DecoderFrame::deallocateParsedFrameData()
{
    if (m_pParsedFrameData)
    {
        // Free the old buffer.
        ippsFree_x(m_pParsedFrameData);
        m_pParsedFrameData = 0;
        m_pRefPicList = 0;
    }

    // new structure(s) hold pointer
    if (m_pParsedFrameDataNew)
    {
        ippsFree_x(m_pParsedFrameDataNew);
        m_pParsedFrameDataNew = NULL;
    }

    m_paddedParsedFrameDataSize = sDimensions(0,0);

}    // deallocateParsedFrameData

Status
H264VideoDecoder::H264DecoderFrame::allocateParsedFrameData(const sDimensions &size,Ipp8u bpp)
{
    Status      umcRes = UMC_OK;
    sDimensions  desiredPaddedSize;

    desiredPaddedSize.width  = (size.width  + 15) & ~15;
    desiredPaddedSize.height = (size.height + 15) & ~15;

    // If our buffer and internal pointers are already set up for this
    // image size, then there's nothing more to do.

    if (m_paddedParsedFrameDataSize != desiredPaddedSize || m_bpp!=bpp)
    {
        //
        // Determine how much space we need
        //
        Ipp32u     MB_Frame_Width   = desiredPaddedSize.width >> 4;
        Ipp32u     MB_Frame_Height  = desiredPaddedSize.height >> 4;


        Ipp32u     uMaxNumSlices = MIN(((Ipp32u) MAX_SLICE_NUM), ((Ipp32u) MB_Frame_Width * MB_Frame_Height));

        Ipp32u        uRefPicListSize = uMaxNumSlices * sizeof(H264DecoderRefPicList);


        Ipp32u     totalSize = Ipp32u(
                    + uRefPicListSize + 7
                    + YUV_ALIGNMENT);

        deallocateParsedFrameData();

        int len = MAX(1, totalSize);

        m_pParsedFrameData = ippsMalloc_8u_x(len);

        if (!m_pParsedFrameData)
            return UMC_FAILED_TO_ALLOCATE_BUFFER;

        ippsZero_8u_x(m_pParsedFrameData, len);

        // Reassign our internal pointers
        {
            m_paddedParsedFrameDataSize = desiredPaddedSize;

            Ipp8u     *pAlignedParsedData;
            Ipp32u     offset = 0;

            pAlignedParsedData = _ALIGN(m_pParsedFrameData, YUV_ALIGNMENT);

            // align to 8-byte boundary
            //if (offset & 0x7)
            //    offset = (offset + 7) & ~7;
            m_pRefPicList = (H264DecoderRefPicList*)(pAlignedParsedData + offset);
            offset += uRefPicListSize;

            VM_ASSERT(offset <= totalSize);
        }

        // allocate new MB structure(s)
        {
            size_t nMBCount = (desiredPaddedSize.width>>4) * (desiredPaddedSize.height>>4);

            // allocate buffer
            size_t nMemSize = (sizeof(H264DecoderMacroblockMVs) +
                               sizeof(H264DecoderMacroblockMVs) +
                               sizeof(H264DecoderMacroblockRefIdxs) +
                               sizeof(H264DecoderMacroblockRefIdxs) +
                               sizeof(H264DecoderMacroblockGlobalInfo)) * nMBCount + 16 * 5;

            // allocate buffer
            m_pParsedFrameDataNew = ippsMalloc_8u_x((Ipp32s) nMemSize);

            if (NULL == m_pParsedFrameDataNew)
                return UMC_FAILED_TO_ALLOCATE_BUFFER;

            ippsZero_8u_x(m_pParsedFrameDataNew, (Ipp32s) nMemSize);

            // set pointer(s)
            m_mbinfo.MV[0] = align_pointer<H264DecoderMacroblockMVs *> (m_pParsedFrameDataNew, ALIGN_VALUE);
            m_mbinfo.MV[1] = align_pointer<H264DecoderMacroblockMVs *> (m_mbinfo.MV[0]+ nMBCount, ALIGN_VALUE);
            m_mbinfo.RefIdxs[0] = align_pointer<H264DecoderMacroblockRefIdxs *> (m_mbinfo.MV[1] + nMBCount, ALIGN_VALUE);
            m_mbinfo.RefIdxs[1] = align_pointer<H264DecoderMacroblockRefIdxs *> (m_mbinfo.RefIdxs[0] + nMBCount, ALIGN_VALUE);
            m_mbinfo.mbs = align_pointer<H264DecoderMacroblockGlobalInfo *> (m_mbinfo.RefIdxs[1] + nMBCount, ALIGN_VALUE);
        }

    }

    return umcRes;

} // H264VideoDecoder::H264DecoderFrame::allocateParsedFrameData(const sDimensions &size)

H264VideoDecoder::H264DecoderFrame::~H264DecoderFrame()
{
    // Just to be safe.
    m_pPreviousFrame = 0;
    m_pFutureFrame = 0;
    deallocateParsedFrameData();
}

Status
H264VideoDecoder::H264DecoderFrame::allocate(const sDimensions &lumaSize,Ipp8u bpp,Ipp8u chroma_format)
{
    Status      umcRes = UMC_OK;

    // Clear our state, since allocate is called when we are about
    // to decode into this frame buffer.

    m_wasOutputted = false;
    m_isDisplayable = false;
    m_cumulativeTR = 0.0;
    m_dimensions = lumaSize;

    // Don't reset m_activeReference or m_lockedForDisplay as these are handled
    // depending on frame type or by the calling application, respectively

    umcRes = allocateParsedFrameData(lumaSize,bpp);
    if (umcRes == UMC_OK)
        umcRes = H264DecYUVWorkSpace::allocate(lumaSize,bpp,chroma_format);

    return umcRes;
}



//////////////////////////////////////////////////////////////////////////////
// H264Decoder constructor
//////////////////////////////////////////////////////////////////////////////
H264VideoDecoder::H264VideoDecoder ()
    :
	/*m_pFrameLevel_BitStream(0), */
    m_pParsedData(0)
    , m_pParsedDataNew(0)
    , m_parsedDataLength(0)

    /*, m_pMBIntraTypes(0)*/
    //***bnie: , m_field_index_kinoma_always_zero(0)
    , m_bSeqParamSetRead(false)
    , m_bPicParamSetRead(false)
    , m_CurrentSeqParamSet(-1)
    , m_CurrentPicParamSet(-1)
    , m_bDPBSizeChanged(false)
    , m_bSeqParamSetChanged(true)
    , m_WaitForDR(true)

	, m_initialTR(0.0)
    , m_dpbSize(1)

    , m_pCurrentFrame(0)
    , m_pDisplayFrame(0)
    , m_broken_buffer(0)
    , m_broken_buffer_start_mb(0)
    , m_broken_buffer_start_slice(0)
	/* DEBUG : skipping was turned OFF, require to reimplement
    , m_getframe_calls(0)
    , m_SkipThisPic(false)
    , m_PreviousPicSkipped(false)*/
	
	//***kinoma modification
	//***bnie: , m_maff(0)

	, m_approx(0)
    , m_SkipFlag(0)
    , m_SkipCycle(1)
    , m_ModSkipCycle(1)
    , m_VideoDecodingSpeed(0)
    , m_bIsDecodingStarted(false)
{

    int i;

    //***m_local_delta_frame_time = 1.0/30;
    //***m_local_frame_time         = 0;

    for (i=0; i<MAX_NUM_SEQ_PARAM_SETS; i++)
    {
        m_SeqParamSet[i].poffset_for_ref_frame = NULL;
    }
    for (i=0; i<MAX_NUM_PIC_PARAM_SETS; i++)
    {
        m_PicParamSet[i].pFMOMap = NULL;
    }

    ulResized = 0;

    m_pSliceStore = NULL;
    m_pSliceDecoder = NULL;
    m_iSliceDecoderNum = 0;

    // set conversion threading tools
    //*** vm_event_set_invalid(&m_hStartConversion);
    //*** vm_event_set_invalid(&m_hStopConversion);
    //*** vm_thread_set_invalid(&m_hConversionThread);
}

//////////////////////////////////////////////////////////////////////////////
// Start_Sequence: This method should be called if there are significant
//                 changes in the input format in the compressed bit stream.
//                 This method must be called between construction
//                 of the H264Decoder object and the first time a decompress
//                 is done.
//////////////////////////////////////////////////////////////////////////////
Status H264VideoDecoder::Init(BaseCodecParams_V51 *pInit)
{
    Status umcRes = UMC_OK;
    Ipp32s i;
    sDimensions dimensions(0,0);
    VideoDecoderParams_V51 *init = DynamicCast<VideoDecoderParams_V51> (pInit);
    Ipp32s nAllowedThreadNumber;

	// check pInit is NULL?
	if(NULL == init)
		return UMC_FAILED_TO_INITIALIZE;

    // release object before initialization
    Close();

    m_bSeqParamSetRead  = false;
    m_bPicParamSetRead  = false;
    m_ReallySkipped     = 0;
    m_AskedSkipped      = 0;
    m_NeedToSkip        = 0;
    m_SkipRepeat        = 0;
    m_PermanentTurnOffDeblocking = 0;
    m_getframe_calls    = 0;
    m_bIsDecodingStarted= false;
    //***bnie: m_field_index_kinoma_always_zero       = 0;
    m_WaitForDR         = true;
    //***m_bHasSEI           = false;

    for (i=0; i<MAX_NUM_SEQ_PARAM_SETS; i++)
    {
        m_SeqParamSet[i].poffset_for_ref_frame = NULL;
        m_SeqParamSet[i].seq_parameter_set_id = MAX_NUM_SEQ_PARAM_SETS;    // illegal id
    }
    for (i=0; i<MAX_NUM_PIC_PARAM_SETS; i++)
    {
        m_PicParamSet[i].pFMOMap = NULL;
        m_PicParamSet[i].pic_parameter_set_id = MAX_NUM_PIC_PARAM_SETS;    // illegal id
        m_PicParamSet[i].num_slice_groups = 0;
    }

    //for (; umcRes == UMC_OK; )
    // Not really a loop; use break instead of goto on error
    do{
        Ipp32s k;

        for (k = 0; k < DEC_NUM_ALLOC_REF_FRAMES; k++)
        {
            H264DecoderFrame *pFrame = new H264DecoderFrame();
            if (!pFrame)
            {
                umcRes = UMC_FAILED_TO_ALLOCATE_BUFFER;
                break;
            }
            //m_freeReferenceFrames.append(pFrame);
            m_H264DecoderFramesList.append(pFrame);
        }

        if (umcRes != UMC_OK)
            break;

        //Status res = UMC_OK;
        //***bnie: m_pFrameLevel_BitStream = new H264Bitstream();
        //***bnie: if (!m_pFrameLevel_BitStream)
        //***bnie:     res = UMC_FAILED_TO_ALLOCATE_BUFFER;
        //***bnie: if (res != UMC_OK)
        //***bnie: {
        //***bnie:     delete m_pFrameLevel_BitStream;
        //***bnie:     m_pFrameLevel_BitStream = NULL;
        //***bnie:     break;
        //***bnie: }

        break;
    }while(0);


    m_lFlags = init->lFlags;


///////////////////////////////////

    for (;;)
    // Not really a loop.  Use "break" instead of "goto" on error.
    {

        if (umcRes != UMC_OK)
            break;

        m_pDisplayFrame = 0;
        m_pCurrentFrame = 0;
            // Any previously displayed or decoded frame
            // will no longer be valid.

        // Reallocate all of our buffers to the exact size needed.
        // To reduce memory fragmentation, delete all the memory before
        // reallocating any.  But, for performance reasons, try not to
        // delete anything that's already the correct size.  Remember,
        // Start_Sequence might get called during a seek operation, so
        // performance is a concern.

        {

            umcRes = AllocateParsedData(dimensions, true);
            if (umcRes != UMC_OK)
                break;

        }

        break;
    }

    //
    // Threading tools
    //

    // get allowed thread numbers
    nAllowedThreadNumber = init->uiLimitThreads;
    if(nAllowedThreadNumber < 0) nAllowedThreadNumber = 0;
    if(nAllowedThreadNumber > 32) nAllowedThreadNumber = 32;

    // calculate number of slice decoders.
    // It should be equal to CPU number
#ifndef DROP_MULTI_THREAD
    m_iSliceDecoderNum = (0 == nAllowedThreadNumber) ? (vm_sys_info_get_cpu_num()) : (nAllowedThreadNumber);
#else
    m_iSliceDecoderNum = nAllowedThreadNumber;
#endif

    m_pSliceStore = new H264SliceStore_(m_PicParamSet, m_SeqParamSet, &m_H264DecoderFramesList);

	if ((NULL == m_pSliceStore) ||
        (false == m_pSliceStore->Init(m_iSliceDecoderNum)))
        return UMC_FAILED_TO_INITIALIZE;

    // create slice decoder(s)
    m_pSliceDecoder = new H264SliceDecoder *[m_iSliceDecoderNum];
    if (NULL == m_pSliceDecoder)
        return UMC_ALLOC;
    memset(m_pSliceDecoder, 0, sizeof(H264SliceDecoder *) * m_iSliceDecoderNum);
    for (i = 0; i < m_iSliceDecoderNum; i += 1)
    {
#ifndef DROP_MULTI_THREAD
#ifndef MULTI_SLICE_MODE
        if (1 != m_iSliceDecoderNum)
            m_pSliceDecoder[i] = new H264SliceDecoderMultiThreaded(*m_pSliceStore);
        else
#endif // MULTI_SLICE_MODE
#endif
            m_pSliceDecoder[i] = new H264SliceDecoder(*m_pSliceStore);
        if (NULL == m_pSliceDecoder[i])
            return UMC_ALLOC;
    }
    for (i = 0;i < m_iSliceDecoderNum;i += 1)
    {
        if (UMC_OK != m_pSliceDecoder[i]->Init(i))
            return UMC_FAILED_TO_INITIALIZE;
    }

    //
    // End of threading tools
    //

	if(init->m_pData3)
		umcRes = DecodeHeaders(init->m_pData3); 
		//umcRes = GetFrame((MediaData *) init->m_pData, NULL);//umcRes;

    if((UMC_NOT_ENOUGH_DATA == umcRes)) // && !(m_lFlags & FLAG_VDEC_REORDER))
        umcRes = UMC_OK;

	///*** WWD_VER51
	//***bnie: m_maff = (m_SeqParamSet[0].frame_mbs_only_flag_kinoma_always_one != 1);
	m_pCurrentFrame_shadow = NULL;

    return umcRes;
}


Status H264VideoDecoder::Close()
{
    // release slice decoders
    if (m_pSliceDecoder)
    {
        Ipp32s i;

        for (i = 0; i < m_iSliceDecoderNum; i += 1)
        {
            if (m_pSliceDecoder[i])
                delete m_pSliceDecoder[i];
        }
        delete [] m_pSliceDecoder;
    }
    m_pSliceDecoder = NULL;
    m_iSliceDecoderNum = 0;

    delete m_pSliceStore;

    //***bnie: if(m_pFrameLevel_BitStream)
    //***bnie:     delete m_pFrameLevel_BitStream;
    //***bnie: m_pFrameLevel_BitStream = NULL;

    DeallocateParsedData();

    Ipp32u i;
    for (i=0; i<MAX_NUM_SEQ_PARAM_SETS; i++)
    {
        if (m_SeqParamSet[i].poffset_for_ref_frame)
        {
            ippsFree_x(m_SeqParamSet[i].poffset_for_ref_frame);
            m_SeqParamSet[i].poffset_for_ref_frame = NULL;
        }
    }
    for (i=0; i<MAX_NUM_PIC_PARAM_SETS; i++)
    {
        if (m_PicParamSet[i].pFMOMap)
        {
            ippsFree_x(m_PicParamSet[i].pFMOMap);
            m_PicParamSet[i].pFMOMap = NULL;
        }
    }

    return UMC_OK;
}


//////////////////////////////////////////////////////////////////////////////
// H264Decoder Destructor
//////////////////////////////////////////////////////////////////////////////
H264VideoDecoder::~H264VideoDecoder(void)
{
    Close();
}

//////////////////////////////////////////////////////////////////////////////
// UpdateRefPicList
//
// Management of active reference picture lists to be used for decoding the
// slice whose header has just been read.
//
//////////////////////////////////////////////////////////////////////////////
Status H264VideoDecoder::UpdateRefPicList(
    H264SliceHeader SHdr,
    RefPicListReorderInfo *pReorderInfo_L0,
    RefPicListReorderInfo *pReorderInfo_L1,
    Ipp32u uSliceNum
)
{
    Status umcRes = UMC_OK;
    H264SeqParamSet *sps;
    H264PicParamSet *pps;
    Ipp32u uMaxFrameNum;
    Ipp32u uMaxPicNum;
    H264DecoderFrame *pFrm;
    H264DecoderFrame *pHead = m_H264DecoderFramesList.head();
    //Ipp32u i;
    H264DecoderFrame **pRefPicList0;
    H264DecoderFrame **pRefPicList1;
    Ipp32u NumShortTermRefs, NumLongTermRefs;

    VM_ASSERT(m_pCurrentFrame);

	//***kinoma enhancement   --bnie 8/2/2008
	pRefPicList0 = m_pCurrentFrame->GetRefPicListSafe( uSliceNum, 0 );
	pRefPicList1 = m_pCurrentFrame->GetRefPicListSafe( uSliceNum, 1 );
		
	//pRefPicList0 = m_pCurrentFrame->GetRefPicList(uSliceNum, 0)->m_RefPicList;
    //pRefPicList1 = m_pCurrentFrame->GetRefPicList(uSliceNum, 1)->m_RefPicList;
    //pFields0 = m_pCurrentFrame->GetRefPicList(uSliceNum, 0)->m_Prediction;
    //pFields1 = m_pCurrentFrame->GetRefPicList(uSliceNum, 1)->m_Prediction;

    // Spec reference: 8.2.4, "Decoding process for reference picture lists
    // construction"

    // get pointers to the picture and sequence parameter sets currently in use
    pps = &m_PicParamSet[SHdr.pic_parameter_set_id];
    sps = &m_SeqParamSet[pps->seq_parameter_set_id];
    uMaxFrameNum = (1<<sps->log2_max_frame_num);
    uMaxPicNum = uMaxFrameNum;//***bnie: (SHdr.field_pic_flag == 0) ? uMaxFrameNum : uMaxFrameNum<<1;

    for (pFrm = pHead; pFrm; pFrm = pFrm->future())
    {
        // update FrameNumWrap and PicNum if frame number wrap occurred,
        // for short-term frames
        // TBD: modify for fields
        pFrm->UpdateFrameNumWrap((Ipp32s)SHdr.frame_num, uMaxFrameNum, m_pCurrentFrame->m_PictureStructureForRef/***bnie: +m_pCurrentFrame->m_bottom_field_flag[0]*/);

        // For long-term references, update LongTermPicNum. Note this
        // could be done when LongTermFrameIdx is set, but this would
        // only work for frames, not fields.
        // TBD: modify for fields
        pFrm->UpdateLongTermPicNum(m_pCurrentFrame->m_PictureStructureForRef/***bnie: +m_pCurrentFrame->m_bottom_field_flag[0]*/);
    }

    if ((SHdr.slice_type != INTRASLICE) && (SHdr.slice_type != S_INTRASLICE))
    {
        // Detect and report no available reference frames
        m_H264DecoderFramesList.countActiveRefs(NumShortTermRefs, NumLongTermRefs);
        if ((NumShortTermRefs + NumLongTermRefs) == 0)
        {
            VM_ASSERT(0);
            umcRes = UMC_BAD_STREAM;
        }

        if (umcRes == UMC_OK)
        {
            // Initialize the reference picture lists
            // Note the slice header get function always fills num_ref_idx_lx_active
            // fields with a valid value; either the override from the slice
            // header in the bitstream or the values from the pic param set when
            // there is no override.
            if ((SHdr.slice_type == PREDSLICE) || (SHdr.slice_type == S_PREDSLICE))
                InitPSliceRefPicList( SHdr.num_ref_idx_l0_active,  pRefPicList0);
            else
            {
                //pRefPicList1 = m_pCurrentFrame->GetRefPicList(uSliceNum, 1)->m_RefPicList;
                InitBSliceRefPicLists(SHdr.num_ref_idx_l0_active, SHdr.num_ref_idx_l1_active, pRefPicList0, pRefPicList1);
            }

            // Reorder the reference picture lists
            //Ipp8u num_ST_Ref0 = 0, num_LT_Ref = 0;

            //***if (BPREDSLICE == SHdr.slice_type)
            //***{
            //***}
            if (pReorderInfo_L0->num_entries > 0)
                ReOrderRefPicList(pRefPicList0,pReorderInfo_L0, uMaxPicNum, SHdr.num_ref_idx_l0_active);
            if (BPREDSLICE == SHdr.slice_type)
            {
                if (pReorderInfo_L1->num_entries > 0)
                    ReOrderRefPicList(pRefPicList1,pReorderInfo_L1, uMaxPicNum, SHdr.num_ref_idx_l1_active);
            }
            // If B slice, init scaling factors array
            if ((BPREDSLICE == SHdr.slice_type) && (pRefPicList1[0] != NULL))
                InitDistScaleFactor(SHdr.num_ref_idx_l0_active,pRefPicList0, pRefPicList1);

        }
    }

    return umcRes;

}    // UpdateRefPicList

//////////////////////////////////////////////////////////////////////////////
// InitPSliceRefPicList
//////////////////////////////////////////////////////////////////////////////

void H264VideoDecoder::InitPSliceRefPicList(
    Ipp32s /*NumL0RefActive*/,
    H264DecoderFrame **pRefPicList    // pointer to start of list 0
)
{
    Ipp32s i, j, k;
    Ipp32s NumFramesInList;
    H264DecoderFrame *pHead = m_H264DecoderFramesList.head();
    H264DecoderFrame *pFrm;
    Ipp32s PicNum;
    bool bError = false;

    VM_ASSERT(pRefPicList);

    for (i=0; i<MAX_NUM_REF_FRAMES; i++)
    {
        pRefPicList[i] = NULL;
    }
    NumFramesInList = 0;

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
    }

    // If the number of reference pictures on the L0 list is greater than the
    // number of active references, discard the "extras".
    //I realy don't know why...
    /*if (NumFramesInList > NumL0RefActive)
    {
        for (i=NumFramesInList-1; i>=NumL0RefActive; i--)
            pRefPicList[i] = NULL;
    }*/

}    // InitPSliceRefPicList

//////////////////////////////////////////////////////////////////////////////
// InitBSliceRefPicLists
//////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::InitBSliceRefPicLists(
    Ipp32s /*NumL0RefActive*/,
    Ipp32s /*NumL1RefActive*/,
    H264DecoderFrame **pRefPicList0,    // pointer to start of list 0
    H264DecoderFrame **pRefPicList1        // pointer to start of list 1
)
{
    Ipp32s i, j, k;
    Ipp32s NumFramesInL0List;
    Ipp32s NumFramesInL1List;
    Ipp32s NumFramesInLTList;
    H264DecoderFrame *pHead = m_H264DecoderFramesList.head();
    H264DecoderFrame *pFrm;
    Ipp32s FrmPicOrderCnt;
    H264DecoderFrame *LTRefPicList[MAX_NUM_REF_FRAMES];    // temp storage for long-term ordered list
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
                    while (j<NumFramesInL0List && (pRefPicList0[j]->PicOrderCnt_kinoma_always_frame() > FrmPicOrderCnt))
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
                    while (j<NumFramesInL1List && pRefPicList1[j]->PicOrderCnt_kinoma_always_frame() < FrmPicOrderCnt)
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
    m_NumFramesInL0List = NumFramesInL0List;
    m_NumFramesInL1List = NumFramesInL1List;
    m_NumFramesInLTList = NumFramesInLTList;

}    // InitBSliceRefPicLists

//////////////////////////////////////////////////////////////////////////////
// InitDistScaleFactor
//  Calculates the scaling factor used for B slice temporal motion vector
//  scaling and for B slice bidir predictin weighting using the picordercnt
//  values from the current and both reference frames, saving the result
//  to the DistScaleFactor array for future use. The array is initialized
//  with out of range values whenever a bitstream unit is received that
//  might invalidate the data (for example a B slice header resulting in
//  modified reference picture lists). For scaling, the list1 [0] entry
//    is always used.
//////////////////////////////////////////////////////////////////////////////
#define CalculateDSF(index)                                                     \
        /* compute scaling ratio for temporal direct and implicit weighting*/   \
        tb = picCntCur - picCntRef0;    /* distance from previous */            \
        td = picCntRef1 - picCntRef0;    /* distance between ref0 and ref1 */   \
                                                                                \
        /* special rule: if td is 0 or if L0 is long-term reference, use */     \
        /* L0 motion vectors and equal weighting.*/                             \
        if (td == 0 || pRefPicList0[index]->isLongTermRef_kinoma_always_frame())					\
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
void H264VideoDecoder::InitDistScaleFactor
(
    Ipp32s NumL0RefActive,
    H264DecoderFrame **pRefPicList0,
    H264DecoderFrame **pRefPicList1
)
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
    pDistScaleFactor = m_CurSliceInfo.DistScaleFactor;        //frames or fields
    pDistScaleFactorMV = m_CurSliceInfo.DistScaleFactorMV;  //frames or fields
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

}    // InitDistScaleFactor

//////////////////////////////////////////////////////////////////////////////
// ReOrderRefPicList
//  Use reordering info from the slice header to reorder (update) L0 or L1
//  reference picture list.
//////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::ReOrderRefPicList(
    H264DecoderFrame **pRefPicList,
    RefPicListReorderInfo *pReorderInfo,
    Ipp32s MaxPicNum,
    Ipp32s NumRefActive
)
{
    Ipp32u i;
    Ipp32s j;
    Ipp32s PicNumNoWrap;
    Ipp32s PicNum;
    Ipp32s PicNumPred;
    Ipp32s PicNumCurr;
    H264DecoderFrame *tempFrame[2];
    Ipp32u NumDuplicates;

    // Reference: Reordering process for reference picture lists, 8.2.4.3
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
}    // ReOrderRefPicList

//////////////////////////////////////////////////////////////////////////////
// updateRefPicMarking
//  Called at the completion of decoding a frame to update the marking of the
//  reference pictures in the decoded frames buffer.
//////////////////////////////////////////////////////////////////////////////
Status H264VideoDecoder::UpdateRefPicMarking()
{
    Status umcRes = UMC_OK;
#ifndef DROP_ADAPTIVE_REF_MARKING
    Ipp32u arpmmf_idx;
    Ipp32s PicNum;
    Ipp32s LongTermFrameIdx;
#endif
    bool bCurrentisST = true;
    Ipp32u NumShortTermRefs, NumLongTermRefs;

    if (m_pCurrentFrame->m_bIsIDRPic)
    {
        // mark all reference pictures as unused
        m_H264DecoderFramesList.removeAllRef();
        m_H264DecoderFramesList.IncreaseRefPicListResetCount(m_pCurrentFrame);

        if (m_FirstSliceHeader->long_term_reference_flag)
        {
			m_pCurrentFrame->SetisLongTermRef_kinoma_always_frame();
            m_MaxLongTermFrameIdx = 0;
        }
        else
        {
            m_pCurrentFrame->SetisShortTermRef_kinoma_always_frame();
            m_MaxLongTermFrameIdx = -1;        // no long term frame indices
        }
    }
    else
    {
#ifndef DROP_ADAPTIVE_REF_MARKING
        Ipp32s LastLongTermFrameIdx = -1;
#endif
        // not IDR picture
        if (m_FirstSliceHeader->adaptive_ref_pic_marking_mode_flag == 0)
        {
            // sliding window ref pic marking

            // find out how many active reference frames currently in decoded
            // frames buffer
            m_H264DecoderFramesList.countActiveRefs(NumShortTermRefs, NumLongTermRefs);
            if (((NumShortTermRefs + NumLongTermRefs) >= m_SeqParamSet[m_CurrentSeqParamSet].num_ref_frames) )
            {
                // mark oldest short term reference as unused
                VM_ASSERT(NumShortTermRefs > 0);
                m_H264DecoderFramesList.freeOldestShortTermRef();
            }
        }    // sliding window ref pic marking
#ifndef DROP_ADAPTIVE_REF_MARKING
		else
        {
            // adaptive ref pic marking
            if (m_FrameLevel_AdaptiveMarkingInfo.num_entries > 0)
            {
                for (arpmmf_idx=0; arpmmf_idx<m_FrameLevel_AdaptiveMarkingInfo.num_entries;
                     arpmmf_idx++)
                {
                    switch (m_FrameLevel_AdaptiveMarkingInfo.mmco[arpmmf_idx])
                    {
                    case 1:
                        // mark a short-term picture as unused for reference
                        // Value is difference_of_pic_nums_minus1
                        PicNum = m_pCurrentFrame->PicNum(m_field_index) - (m_FrameLevel_AdaptiveMarkingInfo.value[arpmmf_idx*2] + 1);
                        m_H264DecoderFramesList.freeShortTermRef(PicNum);
                        break;
                    case 2:
                        // mark a long-term picture as unused for reference
                        // value is long_term_pic_num
                        PicNum = m_FrameLevel_AdaptiveMarkingInfo.value[arpmmf_idx*2];
                        m_H264DecoderFramesList.freeLongTermRef(PicNum);
                        break;
                    case 3:
                        // Assign a long-term frame idx to a short-term picture
                        // Value is difference_of_pic_nums_minus1 followed by
                        // long_term_frame_idx. Only this case uses 2 value entries.
                        PicNum = m_pCurrentFrame->PicNum(m_field_index) -
                            (m_FrameLevel_AdaptiveMarkingInfo.value[arpmmf_idx*2] + 1);
                        LongTermFrameIdx = m_FrameLevel_AdaptiveMarkingInfo.value[arpmmf_idx*2+1];

                        // First free any existing LT reference with the LT idx
                        if (LastLongTermFrameIdx !=LongTermFrameIdx) //this is needed since both fields may have equal Idx
                            m_H264DecoderFramesList.freeLongTermRefIdx(LongTermFrameIdx);

                        m_H264DecoderFramesList.changeSTtoLTRef(PicNum, LongTermFrameIdx);
                        LastLongTermFrameIdx = LongTermFrameIdx;
                        break;
                    case 4:
                        // Specify max long term frame idx
                        // Value is max_long_term_frame_idx_plus1
                        // Set to "no long-term frame indices" (-1) when value == 0.
                        m_MaxLongTermFrameIdx = m_FrameLevel_AdaptiveMarkingInfo.value[arpmmf_idx*2] - 1;

                        // Mark any long-term reference frames with a larger LT idx
                        // as unused for reference.
                        m_H264DecoderFramesList.freeOldLongTermRef(m_MaxLongTermFrameIdx);
                        break;
                    case 5:
                        // Mark all as unused for reference
                        // no value
                        m_WaitForDR = false;
                        m_H264DecoderFramesList.removeAllRef();
                        m_H264DecoderFramesList.IncreaseRefPicListResetCount(NULL);
                        m_MaxLongTermFrameIdx = -1;        // no long term frame indices
                        // set "previous" picture order count vars for future
                        ///*m_pCurrentFrame->*/m_PicOrderCntMsb = 0;
                        ///*m_pCurrentFrame->*/m_PicOrderCntLsb = 0;
                        m_FrameNumOffset = 0;
                        m_FrameNum = 0;
                        // set frame_num to zero for this picture, for correct
                        // FrameNumWrap
                        m_pCurrentFrame->setFrameNum(0);
                        m_pCurrentFrame->setFrameNum(0);
                        // set POC to zero for this picture, for correct display order
                        //m_pCurrentFrame->setPicOrderCnt(m_PicOrderCnt,0);
                        //m_pCurrentFrame->setPicOrderCnt(m_PicOrderCnt,1);
                        break;
                    case 6:
                        // Assign long term frame idx to current picture
                        // Value is long_term_frame_idx
                        LongTermFrameIdx = m_FrameLevel_AdaptiveMarkingInfo.value[arpmmf_idx*2];

                        // First free any existing LT reference with the LT idx
                        m_H264DecoderFramesList.freeLongTermRefIdx(LongTermFrameIdx);

                        // Mark current
                        m_pCurrentFrame->SetisLongTermRef(m_field_index);
                        m_pCurrentFrame->setLongTermFrameIdx(LongTermFrameIdx);
                        bCurrentisST = false;
                        break;
                    case 0:
                    default:
                        // invalid mmco command in bitstream
                        VM_ASSERT(0);
                        umcRes = UMC_BAD_FORMAT;
                    }    // switch
                }    // for arpmmf_idx
            }

        }    // adaptive ref pic marking
#endif
    }    // not IDR picture
    if (bCurrentisST)
    {
        // set current as short term
        m_pCurrentFrame->SetisShortTermRef_kinoma_always_frame();
    }

#ifdef H264_DEC_DEBUG
    // verify total number of frames marked as reference frames
    // does not exceed num_ref_frames in sequence parameter set.
    m_H264DecoderFramesList.countActiveRefs(NumShortTermRefs, NumLongTermRefs);
    m_H264DecoderFramesList.debugoutActiveRefs();
    if (m_SeqParamSet[m_CurrentSeqParamSet].num_ref_frames)
        VM_ASSERT((NumShortTermRefs+NumLongTermRefs) <=
            m_SeqParamSet[m_CurrentSeqParamSet].num_ref_frames);
#endif    // DEBUG
    return umcRes;
}    // updateRefPicMarking
//////////////////////////////////////////////////////////////////////////////
// ProcessFrameNumGap
//
// A non-sequential frame_num has been detected. If the sequence parameter
// set field gaps_in_frame_num_value_allowed_flag is non-zero then the gap
// is OK and "non-existing" frames will be created to correctly fill the
// gap. Otherwise the gap is an indication of lost frames and the need to
// handle in a reasonable way.
//////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::ProcessFrameNumGap()
{
    Ipp32u frame_num;
    Status umcRes = UMC_OK;
    Ipp32u uMaxFrameNum = (1<<m_SeqParamSet[m_CurrentSeqParamSet].log2_max_frame_num);
    H264DecoderFrame *pFrm;
    H264DecoderFrame *pHead;
    Ipp32u NumShortTermRefs, NumLongTermRefs;
    sDimensions lumaSize;
    lumaSize.width = m_SeqParamSet[m_CurrentSeqParamSet].frame_width_in_mbs * 16;
    lumaSize.height = m_SeqParamSet[m_CurrentSeqParamSet].frame_height_in_mbs * 16;
    H264DecoderFrame *TempCurrent = m_pCurrentFrame;
    m_FrameNumGapPresented = 1;
    if (m_SeqParamSet[m_CurrentSeqParamSet].gaps_in_frame_num_value_allowed_flag)
    {
        // Fill the frame_num gap with non-existing frames. For each missing
        // frame:
        //  - allocate a frame
        //  - set frame num and pic num
        //  - update FrameNumWrap for all reference frames
        //  - use sliding window frame marking to free oldest reference
        //  - mark the frame as short-term reference
        // The picture part of the generated frames is unimportant -- it will
        // not be used for reference.

        // set to first missing frame. Note that if frame number wrapped during
        // the gap, the first missing frame_num could be larger than the
        // current frame_num. If that happened, FrameNumGap will be negative.
        VM_ASSERT((Ipp32s)m_FirstSliceHeader->frame_num > m_FrameNumGap);
        frame_num = (Ipp32u)((Ipp32s)m_FirstSliceHeader->frame_num - m_FrameNumGap);

        while ((frame_num != m_FirstSliceHeader->frame_num) && (umcRes == UMC_OK))
        {
            // allocate a frame
            // Traverse list for next disposable frame
            m_pCurrentFrame = m_H264DecoderFramesList.findNextDisposable();

            // Did we find one?
            if (!m_pCurrentFrame)
            {
                // Didn't find one. Let's try to insert a new one
                H264DecoderFrame *pFrame = new H264DecoderFrame();
                if (!pFrame)
                {
                    umcRes = UMC_ALLOC;
                    break;
                }
                m_H264DecoderFramesList.insertAtCurrent(pFrame);

                // Allocate the frame data.
                umcRes = pFrame->allocate(lumaSize,8,0);
                if (umcRes != UMC_OK)
                {
                    umcRes = UMC_ALLOC;
                    break;
                }

                // Now set the current to the new frame in the list
                m_pCurrentFrame = pFrame;
                m_H264DecoderFramesList.setCurrent(m_pCurrentFrame);
            }

            // Set current as not displayable and not outputted.
            m_pCurrentFrame->unsetWasOutputted();
            m_pCurrentFrame->unSetisDisplayable();
            m_pCurrentFrame->m_crop_left = m_SeqParamSet[m_CurrentSeqParamSet].frame_cropping_rect_left_offset;
            m_pCurrentFrame->m_crop_right = m_SeqParamSet[m_CurrentSeqParamSet].frame_cropping_rect_right_offset;
            m_pCurrentFrame->m_crop_top = m_SeqParamSet[m_CurrentSeqParamSet].frame_cropping_rect_top_offset;//***bnie
                //***bnie*(2-m_SeqParamSet[m_CurrentSeqParamSet].frame_mbs_only_flag);
            m_pCurrentFrame->m_crop_bottom = m_SeqParamSet[m_CurrentSeqParamSet].frame_cropping_rect_bottom_offset;//***bnie
               //***bnie *(2-m_SeqParamSet[m_CurrentSeqParamSet].frame_mbs_only_flag);
            m_pCurrentFrame->m_crop_flag = m_SeqParamSet[m_CurrentSeqParamSet].frame_cropping_flag;


            // Set frame num and pic num for the missing frame
            //m_pCurrentFrame->setFrameNum(frame_num);
            {
				//***bnie: m_pCurrentFrame->m_bottom_field_flag[0] = 0;
                //***bnie: m_pCurrentFrame->m_bottom_field_flag[1] = 1;

                //***bniebnie: 
				m_pCurrentFrame->setPicOrderCnt(m_TopFieldPOC_kinoma_question,0);
                m_pCurrentFrame->setPicOrderCnt(m_BottomFieldPOC_kinoma_question,1);
            }
            m_pCurrentFrame->setFrameNum(frame_num);
            //***bnie: if (m_FirstSliceHeader->field_pic_flag == 0)
                m_pCurrentFrame->setPicNum(frame_num,0);
            //***bnie: else
            //***bnie: {
            //***bnie:     m_pCurrentFrame->setPicNum(frame_num*2+1,0);
            //***bnie:     m_pCurrentFrame->setPicNum(frame_num*2+1,1);
            //***bnie: }
            // Update FrameNumWrap and PicNum for all decoded frames
            pHead = m_H264DecoderFramesList.head();
            for (pFrm = pHead; pFrm; pFrm = pFrm->future())
            {
                // TBD: modify for fields
                pFrm->UpdateFrameNumWrap(frame_num,
                    uMaxFrameNum,
                    m_pCurrentFrame->m_PictureStructureForRef
					//***bnie: +m_pCurrentFrame->m_bottom_field_flag[0]
					);
            }

            // sliding window ref pic marking

            // find out how many active reference frames currently in decoded
            // frames buffer
            m_H264DecoderFramesList.countActiveRefs(NumShortTermRefs, NumLongTermRefs);
            if ((NumShortTermRefs + NumLongTermRefs) >=
                m_SeqParamSet[m_CurrentSeqParamSet].num_ref_frames)
            {
                // mark oldest short term reference as unused
                VM_ASSERT(NumShortTermRefs > 0);
                m_H264DecoderFramesList.freeOldestShortTermRef();
            }

            // mark generated frame as short-term reference
            m_pCurrentFrame->SetisShortTermRef_kinoma_always_frame();

            // next missing frame
            frame_num++;
            if (frame_num >= uMaxFrameNum)
                frame_num = 0;

        }   // while
        m_pCurrentFrame = TempCurrent;
    }   // gaps allowed
    else
    {
        // frame_num gap detected in a sequence for which gaps are not allowed.
        // Something reasonable: mark all references as unused. A later check
        // in the decoder will result in an error when attempting to decode
        // any non-INTRA slice. This will continue until a frame with all
        // INTRA slices is decoded.
        m_H264DecoderFramesList.removeAllRef();

    }   // gaps not allowed

}   // ProcessFrameNumGap

void H264VideoDecoder::DecodePictureOrderCount()
{
    Ipp32u uMaxFrameNum = (1<<m_SeqParamSet[m_CurrentSeqParamSet].log2_max_frame_num);

    // Capture any frame_num gap
    if (m_FirstSliceHeader->frame_num != m_FrameNum &&
        m_FirstSliceHeader->frame_num != (m_FrameNum + 1) % uMaxFrameNum)
        m_FrameNumGap = m_FirstSliceHeader->frame_num - m_FrameNum - 1;
            // note this could be negative if frame num wrapped
    else
        m_FrameNumGap = 0;

    if (m_SeqParamSet[m_CurrentSeqParamSet].pic_order_cnt_type == 0)
    {
        // pic_order_cnt type 0
        Ipp32u CurrPicOrderCntMsb;
        Ipp32u MaxPicOrderCntLsb = m_SeqParamSet[m_CurrentSeqParamSet].MaxPicOrderCntLsb;

        if ((m_FirstSliceHeader->pic_order_cnt_lsb < m_PicOrderCntLsb) &&
             ((m_PicOrderCntLsb - m_FirstSliceHeader->pic_order_cnt_lsb) >= (MaxPicOrderCntLsb >> 1)))
            CurrPicOrderCntMsb = m_PicOrderCntMsb + MaxPicOrderCntLsb;
        else if ((m_FirstSliceHeader->pic_order_cnt_lsb > m_PicOrderCntLsb) &&
                ((m_FirstSliceHeader->pic_order_cnt_lsb - m_PicOrderCntLsb) > (MaxPicOrderCntLsb >> 1)))
            CurrPicOrderCntMsb = m_PicOrderCntMsb - MaxPicOrderCntLsb;
        else
            CurrPicOrderCntMsb = m_PicOrderCntMsb;

        if (m_NALRefIDC_kinoma_one_entry)
        {
            // reference picture
            m_PicOrderCntMsb = CurrPicOrderCntMsb & (~(MaxPicOrderCntLsb - 1));
            m_PicOrderCntLsb = m_FirstSliceHeader->pic_order_cnt_lsb;
        }
        m_PicOrderCnt = CurrPicOrderCntMsb + m_FirstSliceHeader->pic_order_cnt_lsb;
       //***bnie:  if( m_FirstSliceHeader->field_pic_flag==0)
        {
             m_TopFieldPOC_kinoma_question = CurrPicOrderCntMsb + m_FirstSliceHeader->pic_order_cnt_lsb;
             m_BottomFieldPOC_kinoma_question = m_TopFieldPOC_kinoma_question + m_FirstSliceHeader->delta_pic_order_cnt_bottom;
        }

    }    // pic_order_cnt type 0
    else if (m_SeqParamSet[m_CurrentSeqParamSet].pic_order_cnt_type == 1)
    {
        // pic_order_cnt type 1
        Ipp32u    i;
        Ipp32u uAbsFrameNum;    // frame # relative to last IDR pic
        Ipp32u uPicOrderCycleCnt = 0;
        Ipp32u uFrameNuminPicOrderCntCycle = 0;
        Ipp32s ExpectedPicOrderCnt = 0;
        Ipp32s ExpectedDeltaPerPicOrderCntCycle;
        Ipp32u uNumRefFramesinPicOrderCntCycle =
                m_SeqParamSet[m_CurrentSeqParamSet].num_ref_frames_in_pic_order_cnt_cycle;

        if (m_FirstSliceHeader->frame_num < m_FrameNum)
            m_FrameNumOffset += uMaxFrameNum;

        if (uNumRefFramesinPicOrderCntCycle != 0)
            uAbsFrameNum = m_FrameNumOffset + m_FirstSliceHeader->frame_num;
        else
            uAbsFrameNum = 0;

        if ((m_NALRefIDC_kinoma_one_entry == false)  && (uAbsFrameNum > 0))
            uAbsFrameNum--;

        if (uAbsFrameNum)
        {
            uPicOrderCycleCnt = (uAbsFrameNum - 1) /
                    uNumRefFramesinPicOrderCntCycle;
            uFrameNuminPicOrderCntCycle = (uAbsFrameNum - 1) %
                    uNumRefFramesinPicOrderCntCycle;
        }

        ExpectedDeltaPerPicOrderCntCycle = 0;
        for (i=0; i<uNumRefFramesinPicOrderCntCycle; i++)
        {
            ExpectedDeltaPerPicOrderCntCycle +=
                m_SeqParamSet[m_CurrentSeqParamSet].poffset_for_ref_frame[i];
        }

        if (uAbsFrameNum)
        {
            ExpectedPicOrderCnt = uPicOrderCycleCnt * ExpectedDeltaPerPicOrderCntCycle;
            for (i=0; i<=uFrameNuminPicOrderCntCycle; i++)
            {
                ExpectedPicOrderCnt +=
                    m_SeqParamSet[m_CurrentSeqParamSet].poffset_for_ref_frame[i];
            }
        }
        else
            ExpectedPicOrderCnt = 0;

        if (m_NALRefIDC_kinoma_one_entry == false)
            ExpectedPicOrderCnt += m_SeqParamSet[m_CurrentSeqParamSet].offset_for_non_ref_pic;
        m_PicOrderCnt = ExpectedPicOrderCnt + m_FirstSliceHeader->delta_pic_order_cnt[0];
        //***bnie: if( m_FirstSliceHeader->field_pic_flag==0) 
		{
            m_TopFieldPOC_kinoma_question = ExpectedPicOrderCnt + m_FirstSliceHeader->delta_pic_order_cnt[ 0 ];
            m_BottomFieldPOC_kinoma_question= m_TopFieldPOC_kinoma_question + m_SeqParamSet[m_CurrentSeqParamSet].offset_for_top_to_bottom_field + m_FirstSliceHeader->delta_pic_order_cnt[ 1 ];
        }
        //***bnie: else if( ! m_FirstSliceHeader->bottom_field_flag)
        //***bnie:     m_PicOrderCnt = ExpectedPicOrderCnt + m_FirstSliceHeader->delta_pic_order_cnt[ 0 ];
        //***bnie: else
        //***bnie:     m_PicOrderCnt  = ExpectedPicOrderCnt + m_SeqParamSet[m_CurrentSeqParamSet].offset_for_top_to_bottom_field + m_FirstSliceHeader->delta_pic_order_cnt[ 0 ];
    }    // pic_order_cnt type 1
    else if (m_SeqParamSet[m_CurrentSeqParamSet].pic_order_cnt_type == 2)
    {
        // pic_order_cnt type 2
        Ipp32s iMaxFrameNum = (1<<m_SeqParamSet[m_CurrentSeqParamSet].log2_max_frame_num);
        Ipp32u uAbsFrameNum;    // frame # relative to last IDR pic

        if (m_FirstSliceHeader->frame_num < m_FrameNum)
            m_FrameNumOffset += iMaxFrameNum;
        uAbsFrameNum = m_FirstSliceHeader->frame_num + m_FrameNumOffset;
        m_PicOrderCnt = uAbsFrameNum*2;

		//***bniebnie
        if (m_NALRefIDC_kinoma_one_entry == false)
            m_PicOrderCnt--;
            m_TopFieldPOC_kinoma_question = m_PicOrderCnt;
            m_BottomFieldPOC_kinoma_question = m_PicOrderCnt;

    }    // pic_order_cnt type 2

    m_FrameNum = m_FirstSliceHeader->frame_num;
}    // decodePictureOrderCount

//***
Status
H264VideoDecoder::ForceDPBSize(long dpbSize)
{
	m_dpbSize = (Ipp32s)dpbSize;
	
	return UMC_OK;
}


Status
H264VideoDecoder::SetDPBSize()
{
    Status umcRes = UMC_OK;
    Ipp32u MaxDPBx2;
    Ipp32u dpbLevel;
    Ipp32u dpbSize;

    // MaxDPB, per Table A-1, Level Limits
    switch (m_SeqParamSet[m_CurrentSeqParamSet].level_idc)
    {
    case 10:
        MaxDPBx2 = 297;
        break;
    case 11:
        MaxDPBx2 = 675;
        break;
    case 12:
    case 13:
    case 20:
        MaxDPBx2 = 891*2;
        break;
    case 21:
        MaxDPBx2 = 1782*2;
        break;
    case 22:
    case 30:
        MaxDPBx2 = 6075;
        break;
    case 31:
        MaxDPBx2 = 6750*2;
        break;
    case 32:
        MaxDPBx2 = 7680*2;
        break;
    case 40:
    case 41:
    case 42:
        MaxDPBx2 = 12288*2;
        break;
    case 50:
        MaxDPBx2 = 41400*2;
        break;
    case 51:
        MaxDPBx2 = 69120*2;
        break;
    default:
        MaxDPBx2 = 69120*2; //get as much as we may
        //umcRes = UMC_BAD_FORMAT;
    }

    if (umcRes == UMC_OK)
    {
        Ipp32u width, height;

        width = m_SeqParamSet[m_CurrentSeqParamSet].frame_width_in_mbs*16;
        height = m_SeqParamSet[m_CurrentSeqParamSet].frame_height_in_mbs*16;

        dpbLevel = (MaxDPBx2 * 512) / ((width * height) + ((width * height)>>1));
        dpbSize = MIN(16, dpbLevel);
#ifdef REDUCE_DPB_TO_MAX_REF_FRAMES
        dpbSize = MIN(m_SeqParamSet[m_CurrentSeqParamSet].num_ref_frames*2+1 , dpbLevel);
#endif
        if ((Ipp32s) dpbSize != m_dpbSize)
            m_bDPBSizeChanged = true;
        m_dpbSize = dpbSize + 1;
    }

    return umcRes;

}    // setDPBSize


H264VideoDecoder::H264DecoderFrame *H264VideoDecoder::GetFreeFrame(void)
{
    H264DecoderFrame *pFrame;

    // Traverse list for next disposable frame
    pFrame = m_H264DecoderFramesList.findNextDisposable();

    // Did we find one?
    if (NULL == pFrame)
    {
        // Didn't find one. Let's try to insert a new one
        pFrame = new H264DecoderFrame();
        if (NULL == pFrame)
            return NULL;

        m_H264DecoderFramesList.insertAtCurrent(pFrame);
    }

    // Set current as not displayable (yet) and not outputted. Will be
    // updated to displayable after successful decode.
    pFrame->unsetWasOutputted();
    pFrame->unSetisDisplayable();
    pFrame->SetBusy(true);

    return pFrame;

} // H264VideoDecoder::H264DecoderFrame *H264VideoDecoder::GetFreeFrame(void)
/*
void H264VideoDecoder::SetCurrentFrame(H264DecoderFrame *pFrame)
{

} // void H264VideoDecoder::SetCurrentFrame(H264DecoderFrame *pFrame)
*/
Status H264VideoDecoder::PrepareDecodeBuffers(/*RefPicListReorderInfo *pReorderInfoL0,RefPicListReorderInfo *pReorderInfoL1*/)
{
    Status umcRes = UMC_OK;
    H264SeqParamSet *pRefsps = NULL;
    H264PicParamSet *pRefpps = NULL;

    //***bnie: if(!m_field_index_kinoma_always_zero)
    {/*
        m_pCurrentFrame = 0;*/
        //m_pDisplayFrame = 0;
        m_FrameNumGapPresented = 0;/* DEBUG : skipping was turned OFF, require to reimplement
        m_SkipThisPic=false; */
    }
        // These will get initialized below if there is any
        // decoding and/or displaying to do.
	m_NALRefIDC_kinoma_one_entry = false;
    // Not really a loop, just use break instead of goto on error
    do{
        Ipp32s iSQUANT, iSQUANT_S;
        Ipp32s iSliceMBA;
        //Ipp32u  PicCodType;
        sDimensions  dimensions;

        NAL_Unit_Type NALUType;
        Ipp8u uNALStorageIDC;
        bool bReadSliceHeader = false;
        bool bIsIDRPic = false;

        // Read NAL units at start of picture from bitstream, optionally including
        // a sequence parameter set, a picture parameter set, a picture delimiter,
        // filler bytes, and ending with a required slice header. Must be at least
        // one sequence parameter set, preceding at least one picture parameter
        // set, preceding the first slice header, in the bitstream.
        umcRes = UMC_OK;
        while (!bReadSliceHeader && umcRes == UMC_OK)
        {
			NALUType = m_FirstSliceHeader->nal_unit_type;
			uNALStorageIDC = m_FirstSliceHeader->nal_ref_idc;
            
			m_NALRefIDC_kinoma_one_entry = (uNALStorageIDC) ? (true) : (false);
            //if (umcRes != UMC_OK)
            //    break;

            switch(NALUType)
            {
            case NAL_UT_IDR_SLICE:
                bIsIDRPic   = true;
                m_WaitForDR = false;
                // fall thru to read slice header
            case NAL_UT_SLICE:
                if (m_WaitForDR && m_bIsDecodingStarted && m_bPicParamSetRead)// try to decode 1st frame event if it's not IDR
                {
                    pRefsps = &m_SeqParamSet[m_CurrentSeqParamSet];
                    if (pRefsps->num_ref_frames>1)
                    {
                        m_ReallySkipped++;
                        umcRes = UMC_BAD_STREAM;
                        break;
                    }
                }
                else
                {
                    // slice header
                    if (!m_bPicParamSetRead)
                    {
                        // Error if no picture parameter set has been read yet.
                        umcRes = UMC_BAD_FORMAT;
                        break;
                    }

                    // Not all members of the slice header structure are contained in all
                    // slice headers. So start by init all to zero.
                    //***ippsZero_8u_x((Ipp8u*) &SHdr, sizeof (H264SliceHeader));

                    // Read first part of slice header to get pic param set used

                    //***bnie: umcRes = m_pFrameLevel_BitStream->GetSliceHeaderPart1(&SHdr);
					//***bnie:
					//***bnie: m_CurSliceHeader = *m_FirstSliceHeader;
                    if (m_WaitForDR && m_FirstSliceHeader->slice_type != INTRASLICE)
                    {
                        m_ReallySkipped++;

                        umcRes = m_bIsDecodingStarted?UMC_BAD_STREAM:UMC_NOT_ENOUGH_DATA;
                    }
                    if (umcRes != UMC_OK)
                        break;

                    m_WaitForDR = false;
                    if (m_PicParamSet[m_FirstSliceHeader->pic_parameter_set_id].pic_parameter_set_id <
                            MAX_NUM_PIC_PARAM_SETS)
                    {
                        pRefpps = &m_PicParamSet[m_FirstSliceHeader->pic_parameter_set_id];
                        pRefsps = &m_SeqParamSet[pRefpps->seq_parameter_set_id];
                    }
                    else
                    {
                        // error, reference pic param set has not been read
                        umcRes = UMC_BAD_FORMAT;
                        break;
                    }
					
					//***bnie: m_CurSliceHeader = *m_FirstSliceHeader;

                    bReadSliceHeader = true;
/* DEBUG : skipping was turned OFF, require to reimplement
                    if(IS_DECODE_ONLY_INTRA_REF_SLICES(m_NeedToSkip)
                        && m_NALRefIDC[m_field_index]
                        && SHdr.slice_type!=INTRASLICE && !m_PreviousPicSkipped)
                    {
                        if (m_NeedToSkip>0) m_NeedToSkip--;
                        m_ReallySkipped++;
                        m_SkipThisPic = true;
                        //return UMC_NOT_ENOUGH_DATA;
                    }
                    if((IS_SKIP_NONREF_FRAMES_MODE(m_NeedToSkip) || m_VideoDecodingSpeed>1)
                        && !m_NALRefIDC[m_field_index] && !m_PreviousPicSkipped)
                    {
                        if ((m_SkipFlag%m_ModSkipCycle)==0)
                        {
                            m_SkipThisPic = true;
                            if (m_NeedToSkip>0) m_NeedToSkip--;
                            m_ReallySkipped++;
                        }
                        m_SkipFlag++;
                        if (m_SkipFlag>=m_SkipCycle)
                            m_SkipFlag=0;
                        //return UMC_NOT_ENOUGH_DATA;
                    }
                    if(IS_SKIP_DEBLOCKING_MODE
                        && m_NALRefIDC[m_field_index])
                    {
                        //m_iNeedToSkip--;
                        SHdr.disable_deblocking_filter_idc = DEBLOCK_FILTER_OFF;
                    }
                    m_PreviousPicSkipped=false;*/
/*
                    // save slice header info for next compare
                    if(m_PicParamSet[SHdr.pic_parameter_set_id].entropy_coding_mode)
                    {
                        m_pBitStream->InitializeDecodingEngine_CABAC();

                        if(SHdr.slice_type == INTRASLICE)
                        {
                            m_pBitStream->InitializeContextVariablesIntra_CABAC(m_PicParamSet[SHdr.pic_parameter_set_id].pic_init_qp + SHdr.slice_qp_delta);
                        }
                        else
                        {
                            m_pBitStream->InitializeContextVariablesInter_CABAC(m_PicParamSet[SHdr.pic_parameter_set_id].pic_init_qp + SHdr.slice_qp_delta, SHdr.cabac_init_idc);
                        }
                    }*/
                    break;
                }

            default:
                //discard other values
                break;
            }    // switch
        }    // while (!bReadSliceHeader)

        // Note that among all of the other error possibilities from above, not finding
        // a slice header will result in an error exit here, due to error from
        // GetNALUnitType.
        if (umcRes != UMC_OK)
        {
            break;
        }

        // Update decoder state from bitstream fields just read. They may have not
        // changed but is simpler to just update all.
        m_CurrentPicParamSet = m_FirstSliceHeader->pic_parameter_set_id;
        m_CurrentSeqParamSet = m_PicParamSet[m_CurrentPicParamSet].seq_parameter_set_id;


        dimensions.width = m_SeqParamSet[m_CurrentSeqParamSet].frame_width_in_mbs * 16;
        dimensions.height = m_SeqParamSet[m_CurrentSeqParamSet].frame_height_in_mbs * 16;

        if (m_bIsDecodingStarted)
        {
            m_bSeqParamSetChanged = false;
            m_bDPBSizeChanged = false;
        }

        iSliceMBA = m_FirstSliceHeader->first_mb_in_slice;
        if (iSliceMBA != 0)
        {
//            umcRes = UMC_BAD_FORMAT;
//            break;
        }
        if (m_bSeqParamSetChanged)
        {
            // Validate the incoming bitstream's image dimensions.
#if 0//*** original codes
          	umcRes = SetDPBSize();
            //***  need to take care of this from system level --bryan 11/29/2005
#else
			umcRes = ForceDPBSize(2);
#endif
            if (umcRes != UMC_OK)
                break;
        }

        iSQUANT = m_PicParamSet[m_CurrentPicParamSet].pic_init_qp + m_FirstSliceHeader->slice_qp_delta;
        if (iSQUANT < QP_MIN || iSQUANT > QP_MAX)
        {
//            umcRes = UMC_BAD_FORMAT;
            //break;
        }

        iSQUANT_S = m_PicParamSet[m_CurrentPicParamSet].pic_init_qs + m_FirstSliceHeader->slice_qs_delta;
        if (iSQUANT_S < QP_MIN || iSQUANT_S > QP_MAX)
        {
            //umcRes = UMC_BAD_FORMAT;
            //break;
        }

        if (m_FirstSliceHeader->no_output_of_prior_pics_flag)
        {
            // TBD: remove non-reference pictures in the decoded picture buffer
            VM_ASSERT(0);
        }
        if (umcRes == UMC_OK)
        {
            if (bIsIDRPic)
            {
                m_PicOrderCnt = 0;
                m_PicOrderCntMsb = 0;
                m_PicOrderCntLsb = 0;
                m_FrameNum = 0;
                m_FrameNumOffset = 0;
                m_TopFieldPOC_kinoma_question = 0;
                m_BottomFieldPOC_kinoma_question = 0;
            }
            DecodePictureOrderCount();
            if (m_FrameNumGap != 0 && !m_FrameNumGapPresented && !bIsIDRPic)
                ProcessFrameNumGap();

            // PicOrderCnt
        }

        umcRes = AllocateParsedData(dimensions, false);
        if (umcRes != UMC_OK)
            break;

        {
            //***bnie: m_pCurrentFrame->m_bottom_field_flag[0] = 0;//***bnie: m_TopFieldPOC_kinoma_question> m_BottomFieldPOC_kinoma_question;
            //***bnie: m_pCurrentFrame->m_bottom_field_flag[1] = 1;//***bnie: m_TopFieldPOC_kinoma_question<=m_BottomFieldPOC_kinoma_question;
        }
        m_pCurrentFrame->setFrameNum(m_FirstSliceHeader->frame_num);
        //***bnie: if (m_FirstSliceHeader->field_pic_flag == 0)
            m_pCurrentFrame->setPicNum(m_FirstSliceHeader->frame_num,0);
        //***bnie: else
        //***bnie:     m_pCurrentFrame->setPicNum(m_FirstSliceHeader->frame_num*2+1,m_field_index);
        m_pCurrentFrame->m_bIsIDRPic = bIsIDRPic;

        //transfer previosly calculated PicOrdeCnts into current Frame
        {
            m_pCurrentFrame->setPicOrderCnt(m_TopFieldPOC_kinoma_question,0);
            m_pCurrentFrame->setPicOrderCnt(m_BottomFieldPOC_kinoma_question,1);
        }

        // Set the current frame's cumulative TR

        if (!m_bIsDecodingStarted && umcRes==UMC_OK)
        {
            // Record timing information from the very first frame we see
            m_initialTR = Ipp64f(m_PicOrderCnt);

            if (!bIsIDRPic)
            {
                // We haven't received a reference frame yet. Put out a
                // warning message, tell the app to not display, and
                // keep going, hoping that we'll get a key frame
                umcRes = UMC_OK;
                break;
            }
            m_bIsDecodingStarted = true;
        }
        m_pCurrentFrame->setCumulativeTR(Ipp64f(m_PicOrderCnt));

        //break;
    }while(0);
/* DEBUG : skipping was turned OFF, require to reimplement
    if (m_SkipThisPic)
    {
        // make this pic useless
        m_pCurrentFrame->setWasDisplayed();
        m_pCurrentFrame->setWasOutputted();
        m_pCurrentFrame->unSetisDisplayable();
        m_pCurrentFrame->unSetisShortTermRef(m_field_index);
        m_pCurrentFrame->unSetisLongTermRef(m_field_index);
        m_PreviousPicSkipped = true;
        return UMC_NOT_ENOUGH_DATA;
    }*/

    //fill chroma planes incase of 4:0:0
	if(m_CurrentSeqParamSet >= 0) { //Just for safe: coverity 11082
    if (m_SeqParamSet[m_CurrentSeqParamSet].chroma_format_idc==0 && m_pCurrentFrame && m_CurrentSeqParamSet>=0 && m_CurrentSeqParamSet<MAX_NUM_SEQ_PARAM_SETS)
    {
        IppiSize roi;
        roi.width = m_pCurrentFrame->lumaSize().width>>1;
        roi.height = m_pCurrentFrame->lumaSize().height>>1;
        ippiSet_8u_C1R_x(128,m_pCurrentFrame->m_pUPlane,m_pCurrentFrame->pitch(),roi);
        ippiSet_8u_C1R_x(128,m_pCurrentFrame->m_pVPlane,m_pCurrentFrame->pitch(),roi);
    }
	}

    return umcRes;
}

///***
Status H264VideoDecoder::GetYUV(int display_order, YUVPlannar *yuv)
{
	H264DecoderFrame *thisFrame = display_order? m_pDisplayFrame : m_pCurrentFrame_shadow;
	//H264DecoderFrame *thisFrame = m_pCurrentFrame;
	
	if( thisFrame == NULL )
	{
		yuv->isValid = false;
	}
	else
	{
		yuv->isValid 		= true;
		yuv->pixelFormat	= 9;
		yuv->rowBytesY		= thisFrame->pitch();
		yuv->rowBytesCbCr	= thisFrame->pitch();;
		yuv->y				= thisFrame->m_pYPlane;;
		yuv->cb				= thisFrame->m_pUPlane;
		yuv->cr				= thisFrame->m_pVPlane;
		//yuv->maff_flag      = thisFrame->m
		
		GetDimension( &yuv->width16, &yuv->height16, &yuv->left, &yuv->top, &yuv->right, &yuv->bottom );
	}
	
	return UMC_OK;
}

//***
Status H264VideoDecoder::GetDimension( long *width16, long *height16, long *left, long *top, long *right, long *bottom )
{
	if( m_bSeqParamSetRead == false )
		return -1;

	H264SeqParamSet *spsPtr = &m_SeqParamSet[0];//***assuming 0 for now
	
	*width16  = 16 * ( spsPtr->frame_width_in_mbs ); 
	*height16 = 16 *( spsPtr->frame_height_in_mbs );////***bnie * ( 2 - spsPtr->frame_mbs_only_flag);

	if( spsPtr->frame_cropping_flag )
	{
		*left   = 2*spsPtr->frame_cropping_rect_left_offset;
		*right  = *width16 - 2*spsPtr->frame_cropping_rect_right_offset; //actually 1 + rightmost pel
		*top   = 2*spsPtr->frame_cropping_rect_top_offset;
		*bottom = *height16 - 2*spsPtr->frame_cropping_rect_bottom_offset;
	}
	else
	{
		*left   = 0;
		*right  = *width16;
		*top   = 0;
		*bottom = *height16;
	}

	return UMC_OK;
}


Status H264VideoDecoder::GetFrame(MediaData3_V51* src)
{
    Status umcRes = UMC_OK;

    //m_field_index = 0;

    //
    // image conversion. "Before decoding" call
    // prepare SEI messages, call asynchronous frame conversion
    //
    OutputFrame( PREPARE_DECODING);

    //
    // do frame decoding
    //
    if (src)
    {
        //***bnie: umcRes = ParseFrame(src);
        umcRes = DecodeFrame(src);
        // error correction
        if (UMC_OK != umcRes)
        {
            if (UMC_WRN_NOT_CRITICAL_BAD_FORMAT == umcRes)
                umcRes = UMC_OK;
            else
                OutputFrame(ERROR_DECODING);
            return umcRes;
        }

		//
		// image conversion. "After decoding" call
		// do SEI messages, do synchronization or call synchronous frame conversion
		//
        umcRes = OutputFrame(AFTER_DECODING);
        // error correction
        if (UMC_END_OF_STREAM == umcRes)
            umcRes = UMC_OK;
    }
	else
        umcRes = OutputFrame(ABSENT_DECODING);

    //
    // set times, drop used bytes etc.
    //
    if (umcRes != UMC_OK )//***bnie: && m_field_index_kinoma_always_zero==0)
    {
        m_pCurrentFrame = 0;
        // This prevents the application from trying to read the
        // bitstream info corresponding to this broken image.
    }

    if(m_pDisplayFrame || umcRes == UMC_END_OF_STREAM)
        return umcRes;
    //else if ((NULL == m_pDisplayFrame) && (src) && (src->GetDataSize()))
    //    return GetFrame(src, NULL);
    else
        return UMC_NOT_ENOUGH_DATA;

} // Status H264VideoDecoder::GetFrame(MediaData* src, MediaData* dst)

/*
Status get_sps(MediaData3_V51 *pSource, H264SeqParamSet *sps )
{
    int i;
	H264Bitstream  bs;
    Status		   umcRes = UMC_OK;

    for (i = 0; i < (Ipp32s) pSource->count; i++)
    {
		NAL_Unit_Type NALUType = (NAL_Unit_Type)pSource->nalu_type_ary[i];
		bs.Reset((Ipp8u *) (pSource->data_ptr_ary[i]), pSource->bit_offset_ary[i], (Ipp32u) (pSource->size_ary[i]));

        if( NALUType == NAL_UT_SPS )
		{
			umcRes = bs.GetSequenceParamSet(sps);
			if (umcRes != UMC_OK)
				return UMC_BAD_FORMAT;

			break;
		}
	}

    return UMC_OK;
}
*/

Status get_sps_pps(MediaData3_V51 *pSource, H264SeqParamSet *sps, H264PicParamSet *pps)
{
    int				i;
	int				has_sps = 0;
	H264Bitstream	bs;
    Status		   umcRes = UMC_OK;

    for (i = 0; i < (Ipp32s) pSource->count; i++)
    {
		NAL_Unit_Type NALUType = (NAL_Unit_Type)pSource->nalu_type_ary[i];
		bs.Reset((Ipp8u *) (pSource->data_ptr_ary[i]), pSource->bit_offset_ary[i], (Ipp32u) (pSource->size_ary[i]));

        if( NALUType == NAL_UT_SPS )
		{
			umcRes = bs.GetSequenceParamSet(sps);
			if (umcRes != UMC_OK)
				return UMC_BAD_FORMAT;
			
			has_sps = 1;

			if( pps == NULL )
				break;
		}
		else if( NALUType == NAL_UT_PPS && has_sps )
		{
            // set illegal id
            pps->pic_parameter_set_id = MAX_NUM_PIC_PARAM_SETS;
            // Get id
            umcRes = bs.GetPictureParamSetPart1(pps);
            if (UMC_OK == umcRes)
            {
                if (sps->seq_parameter_set_id >= MAX_NUM_SEQ_PARAM_SETS)
                    return UMC_BAD_FORMAT;

                // Get rest of pic param set
                umcRes = bs.GetPictureParamSetPart2(pps, sps);
   				if (umcRes != UMC_OK)
					return UMC_BAD_FORMAT;
			}

			break;
		}

	}

    return UMC_OK;
}


Status H264VideoDecoder::DecodeHeaders(MediaData3_V51 *pSource)
{
    Status umcRes = UMC_OK;
    //Ipp8u uNALStorageIDC;
    H264SeqParamSet sps;
    H264PicParamSet pps;
    H264SeqParamSet *pRefsps = NULL;
    //MediaDataEx_2 *pMedia = DynamicCast<MediaDataEx_2> (pSource);
    //MediaDataEx_2::_MediaDataEx_2 *pMediaEx = pMedia->GetExData();
    H264Bitstream  m_pFrameLevel_BitStream;

    Ipp32s i = 0;
    for (i = 0; i < (Ipp32s) pSource->count; i++)
    {
		NAL_Unit_Type NALUType = (NAL_Unit_Type)pSource->nalu_type_ary[i];
		m_pFrameLevel_BitStream.Reset((Ipp8u *) (pSource->data_ptr_ary[i]), pSource->bit_offset_ary[i], (Ipp32u) (pSource->size_ary[i]));

        if( NALUType == NAL_UT_SPS )
		{
			umcRes = m_pFrameLevel_BitStream.GetSequenceParamSet(&sps);
			if (umcRes == UMC_OK)
			{
				// save parameter set. If overwriting an existing parameter set
				// need to free any stored frame offset storage first
				if (m_SeqParamSet[sps.seq_parameter_set_id].poffset_for_ref_frame)
					ippsFree_x(m_SeqParamSet[sps.seq_parameter_set_id].poffset_for_ref_frame);
				m_SeqParamSet[sps.seq_parameter_set_id] = sps;
				m_bSeqParamSetRead = true;
			}
			else
				return UMC_BAD_FORMAT;
		}
		else if( NALUType == NAL_UT_PPS )
        {
            // set illegal id
            pps.pic_parameter_set_id = MAX_NUM_PIC_PARAM_SETS;

            // Get id
            umcRes = m_pFrameLevel_BitStream.GetPictureParamSetPart1(&pps);
            if (UMC_OK == umcRes)
            {
                if (m_SeqParamSet[pps.seq_parameter_set_id].seq_parameter_set_id >= MAX_NUM_SEQ_PARAM_SETS)
                    return UMC_BAD_FORMAT;

                pRefsps = &m_SeqParamSet[pps.seq_parameter_set_id];

                // Get rest of pic param set
                umcRes = m_pFrameLevel_BitStream.GetPictureParamSetPart2(&pps, pRefsps);
                if (UMC_OK == umcRes)
                {
                    // save parameter set. If overwriting an existing parameter set
 #ifndef DROP_SLICE_GROUP
					// need to free any slice group map storage first
                    if ((1 < m_PicParamSet[pps.pic_parameter_set_id].num_slice_groups) &&
                        (6 == m_PicParamSet[pps.pic_parameter_set_id].SliceGroupInfo.slice_group_map_type) &&
                        (m_PicParamSet[pps.pic_parameter_set_id].SliceGroupInfo.t3.pSliceGroupIDMap))
                        ippsFree_x(m_PicParamSet[pps.pic_parameter_set_id].SliceGroupInfo.t3.pSliceGroupIDMap);
#endif
                    m_PicParamSet[pps.pic_parameter_set_id] = pps;
                    m_bPicParamSetRead = true;
                    // reset current picture parameter set number
                    if (0 > m_CurrentPicParamSet)
                    {
                        m_CurrentPicParamSet = pps.pic_parameter_set_id;
                        m_CurrentSeqParamSet = pps.seq_parameter_set_id;
                    }
                }
				else
					return umcRes;
            }
        }
    }

    return UMC_OK;

} // Status H264VideoDecoder::DecodeHeaders(MediaData_V51 *pSource)

Status H264VideoDecoder::DecodeFrame(MediaData3_V51 *src)
{
    Status umcRes = UMC_OK;
    H264Status h264Res = H264_OK;

    // add source to source store
	m_pSliceStore->m_AdaptiveMarkingInfo_shadow = &this->m_FrameLevel_AdaptiveMarkingInfo;//***bnie: kinoma added 4/1/09
    h264Res = m_pSliceStore->AddSourceData(src);
	m_FirstSliceHeader = m_pSliceStore->GetQueuedSlice(0)->GetSliceHeader();
	
	switch (h264Res)
    {
        // we require free frame for decoding
		case H264_NEED_FREE_FRAME:
			m_pLastDecodedFrame = GetFreeFrame();
			if (NULL == m_pLastDecodedFrame)
				return UMC_BAD_FORMAT;
			// there is no error - break isn't required.

        // we require just reset current decoding frame
		case H264_NEED_FREE_FIELD:
			h264Res = m_pSliceStore->AddFreeFrame(m_pLastDecodedFrame);
			if( h264Res != H264_OK )
				return h264Res;

			m_pCurrentFrame = m_pLastDecodedFrame;
			h264Res = H264_OK;
			break;

        // it is non video data
		case H264_NON_VCL_NAL_UNIT:
			 // normal case
		case H264_OK:
			break;

        // it is frame parallelization, data isn't enough
		case H264_NOT_ENOUGH_DATA:
			return UMC_NOT_ENOUGH_DATA;

		default:
			return UMC_BAD_FORMAT;
    }

    //
    // main decoding cicle
    //
    umcRes = UMC_OK;
    while (UMC_OK == umcRes)
    {
		// h264Res can be H264_NON_VCL_NAL_UNIT and H264_OK ONLY according to above switch 
        switch (h264Res)
        {
            // it is non-video data
			case H264_NON_VCL_NAL_UNIT:
				src = NULL;
				//***bniebniebnie
				break;

				// normal case, usually video data
			case H264_OK:
				break;
#if 0
				// normal case, not enough data for frame parallelization
			case H264_NOT_ENOUGH_DATA:
				return UMC_NOT_ENOUGH_DATA;
#endif

			default:
				return UMC_BAD_FORMAT;
        }

        if( NULL == src )
            return UMC_NOT_ENOUGH_DATA;

		// reset decoder bitstream
		// set current frame
		VM_ASSERT(m_pCurrentFrame);
		if(NULL == m_pCurrentFrame)
			return UMC_BAD_FORMAT; //Just for safe, actually, m_pCurrentFrame must be NOT NULL
		m_H264DecoderFramesList.setCurrent(m_pCurrentFrame);

		/* DEBUG : function's name should be changed to DecodeStreamHeaders */
		umcRes = PrepareDecodeBuffers();
		// when error or
		if ((UMC_OK != umcRes) )
		{
			//Ipp32s iDataSize;

			if (UMC_FAILED_TO_ALLOCATE_BUFFER == umcRes)
				return umcRes;

			if (UMC_NOT_ENOUGH_DATA == umcRes)
				break;
		}

		m_getframe_calls++;

        //
        // start asynchronous frame conversion.
        //
        //***bnie: if (0 == m_field_index_kinoma_always_zero)
            OutputFrame(BEFORE_DECODING);

		umcRes = SetMBMap();
        if (umcRes != UMC_OK)
            break;

        //***bnie: m_pCurrentFrame->m_SliceType[0] = m_FirstSliceHeader->slice_type;

        // set next frame to decode
        m_pSliceStore->SwitchFrame();
        /* DEBUG : temporary call to keep backward compatibility */
        m_pSliceStore->SetMBMap(m_mbinfo.active_next_mb_table);

        {
#ifndef DROP_MULTI_THREAD
            Ipp32s i;

            for (i = 1;i < m_iSliceDecoderNum;i += 1)
                m_pSliceDecoder[i]->StartProcessing();
#endif

        m_pSliceDecoder[0]->SetApprox(m_approx);//*** kinoma modification
		umcRes = m_pSliceDecoder[0]->ProcessSegment();

#ifndef DROP_MULTI_THREAD
            for (i = 1;i < m_iSliceDecoderNum;i += 1)
            {
                Status Res;
                Res = m_pSliceDecoder[i]->WaitForEndOfProcessing();
                if (UMC_OK != Res)
                    umcRes = Res;
            }
#endif

            /* DEBUG: need to correctly set */
            // set bit stream pointer after decoded data
            //***m_pFrameLevel_BitStream.SetDecodedBytes(m_pSliceStore->GetUsedSize());
			//***bnie: m_pSliceStore->GetUsedSize();
			m_pSliceStore->RecycleSlice();
        }

        //***m_clsTimingInfo.m_frame_count +=2;

        if (umcRes == UMC_NOT_ENOUGH_DATA)
        {
            //***src->MoveDataPointer((Ipp32s) src->GetDataSize());
            return umcRes;
        }

        //***bnie: m_pCurrentFrame->m_SliceType[1]=m_FirstSliceHeader->slice_type;
        if (
			UMC_OK == umcRes
			//***&&((m_pCurrentFrame->m_PictureStructureForDec >= FRM_STRUCTURE) || (m_field_index))
			)
            m_pCurrentFrame->SetisDisplayable();

        m_pCurrentFrame->InitRefPicListResetCount_kinoma_always_frame();

        if (m_NALRefIDC_kinoma_one_entry)
        {
            // Update decoded reference frame buffer if this is a reference frame.
            if (umcRes == UMC_OK)
                umcRes = UpdateRefPicMarking();
            else
                UpdateRefPicMarking();//in this case discard return code
            // Expand the just decoded frame for use as reference
			m_pCurrentFrame->expand();
            //***if(m_pCurrentFrame->m_PictureStructureForDec >= FRM_STRUCTURE || m_field_index)
                m_pCurrentFrame->setExpanded();
        }

		//***kinoma added
		m_pCurrentFrame_shadow = m_pCurrentFrame;
        m_pCurrentFrame = NULL;
		break;
    } // main decoding cicle

    if (umcRes == UMC_OK || umcRes == UMC_BAD_STREAM )
	{
         umcRes = UMC_OK;
    }
    else if (umcRes == UMC_BAD_FORMAT && m_NALRefIDC_kinoma_one_entry)//critical error in reference frame
    {
        m_WaitForDR = true;
        return UMC_WRN_NOT_CRITICAL_BAD_FORMAT;
    }
    else
    {
		//***src->MoveDataPointer((Ipp32s) src->GetDataSize());
        return umcRes;
    }

    return UMC_OK;

} // Status H264VideoDecoder::DecodeFrame(MediaData* sr

Status H264VideoDecoder::GetInfo(BaseCodecParams_V51* params)
{
    Status umcRes = UMC_OK;
    VideoDecoderParams_V51 *lpInfo = DynamicCast<VideoDecoderParams_V51> (params);

    if (NULL == lpInfo)
    {   umcRes = UMC_NULL_PTR;  }

    if (UMC_OK == umcRes)
    {   umcRes = VideoDecoder_V51::GetInfo(params); }

    if (UMC_OK == umcRes)
    {
        int index = 0;
        if(m_CurrentSeqParamSet == -1 && !m_bSeqParamSetRead)
        {
            return UMC_NOT_INITIALIZED;
        }
        else if(m_CurrentSeqParamSet == -1)
        {
            index = 0;
        }
        else
        {
            index = m_CurrentSeqParamSet;
        }

        lpInfo->info.clip_info.height = m_SeqParamSet[index].frame_height_in_mbs * 16-
            2*(m_SeqParamSet[index].frame_cropping_rect_top_offset +
            m_SeqParamSet[index].frame_cropping_rect_bottom_offset);//-1*m_SeqParamSet[m_CurrentSeqParamSet].frame_cropping_flag;

        lpInfo->info.clip_info.width = m_SeqParamSet[index].frame_width_in_mbs * 16-
            (4>>1)
			//***bnie: (4>>m_SeqParamSet[index].frame_mbs_only_flag)
			*(m_SeqParamSet[index].frame_cropping_rect_left_offset
			+ m_SeqParamSet[index].frame_cropping_rect_right_offset);//-1*m_SeqParamSet[m_CurrentSeqParamSet].frame_cropping_flag;

        //***if (m_local_delta_frame_time)
        //***    lpInfo->info.framerate = 1.0 / m_local_delta_frame_time;
        //***else
        //***    lpInfo->info.framerate = 0.0;

        if(ulResized)
        {
            lpInfo->info.clip_info.height/= ulResized;
            lpInfo->info.clip_info.width /= ulResized;
        }

        lpInfo->info.stream_type     = H264_VIDEO;
    }

    return umcRes;
}

void H264VideoDecoder::DeallocateParsedData()
{
    if (m_pParsedData)
    {
        // Free the old buffer.
        ippsFree_x(m_pParsedData);
        m_pParsedData = 0;
    }

    if (m_pParsedDataNew)
    {
        ippsFree_x(m_pParsedDataNew);
        m_pParsedDataNew = NULL;
    }

    m_parsedDataLength = 0;
    m_paddedParsedDataSize = sDimensions(0,0);/*
    m_pMBIntraTypes = 0;*/

}

Status H264VideoDecoder::AllocateParsedData(const sDimensions &size,    bool exactSizeRequested)
{
    Status      umcRes = UMC_OK;
    sDimensions  desiredPaddedSize;

    desiredPaddedSize.width  = (size.width  + 15) & ~15;
    desiredPaddedSize.height = (size.height + 15) & ~15;

    // If our buffer and internal pointers are already set up for this
    // image size, then there's nothing more to do.
    // But if exactSizeRequested, we need to see if our existing
    // buffer is oversized, and perhaps reallocate it.

    if (m_paddedParsedDataSize == desiredPaddedSize && !exactSizeRequested )
        return umcRes;

    //
    // Determine how much space we need
    //

    Ipp32s     MB_Frame_Width   = desiredPaddedSize.width >> 4;
    Ipp32s     MB_Frame_Height  = desiredPaddedSize.height >> 4;

    Ipp32s     uMBMapSize = MB_Frame_Width * MB_Frame_Height;
/*
    Ipp32s     macroblockIntraTypeSize = NUM_INTRA_TYPE_ELEMENTS * sizeof(m_pMBIntraTypes[0]);
    macroblockIntraTypeSize *= uMBMapSize;*/
    Ipp32s     next_mb_size=  MB_Frame_Width*MB_Frame_Height*sizeof(H264DecoderMBAddr);

    //Ipp32u        NumCoefSize = Sub_Block_Width * 6 * sizeof(Ipp8u);
            // space for num coeff per 4x4 block, for the 8 (4 luma + 4 chroma)
            // blocks of a macroblock


    Ipp32s     totalSize = Ipp32u(
                    // By putting it first, we get YUV_ALIGMENT, which
                    // is 32 currently.
/*
                + macroblockIntraTypeSize*/
                + 3*next_mb_size
                + uMBMapSize
                + YUV_ALIGNMENT);

    //
    // Reallocate our buffer if its size is not appropriate.
    //

    if (m_parsedDataLength < totalSize
        ||
        (exactSizeRequested && (m_parsedDataLength != totalSize))
       )
    {
        DeallocateParsedData();

        m_pParsedData = ippsMalloc_8u_x(totalSize);
        if (!m_pParsedData)
        {
            return UMC_FAILED_TO_ALLOCATE_BUFFER;
        }
        ippsZero_8u_x(m_pParsedData, totalSize);

        m_parsedDataLength = totalSize;
    }

    // Reassign our internal pointers if need be

    if (m_paddedParsedDataSize != desiredPaddedSize)
    {
        m_paddedParsedDataSize = desiredPaddedSize;

        Ipp8u     *pAlignedParsedData;
        Ipp32u     offset = 0;

        pAlignedParsedData = _ALIGN(m_pParsedData, YUV_ALIGNMENT);

/*
        // m_pMBIntraTypes must be 4-byte aligned, is 16-byte aligned
        m_pMBIntraTypes = (Ipp32u*)(pAlignedParsedData + offset);
        offset += macroblockIntraTypeSize;

        // align to 8-byte boundary
        if (offset & 0x7)
            offset = (offset + 7) & ~7;
*/
        m_pMBMap = (Ipp8u *)(pAlignedParsedData + offset);
        offset += uMBMapSize;

        if (offset & 0x7)
            offset = (offset + 7) & ~7;
        next_mb_tables[0] = (H264DecoderMBAddr *)(pAlignedParsedData + offset);

        offset += next_mb_size;

        if (offset & 0x7)
            offset = (offset + 7) & ~7;

        next_mb_tables[1] = (H264DecoderMBAddr *)(pAlignedParsedData + offset);

        offset += next_mb_size;

        if (offset & 0x7)
            offset = (offset + 7) & ~7;

        next_mb_tables[2] = (H264DecoderMBAddr *)(pAlignedParsedData + offset);
        //initialize first 2 tables
        {
            int i;

            for (i=0;i<uMBMapSize;i++)
                next_mb_tables[0][i]=i+1; // simple linear scan

            for (i=0;i<MB_Frame_Height ;i+=2) //MBAFF Linear scan
            {
                for (int j=0;j<MB_Frame_Width ;j++)
                {
                    next_mb_tables[1][i*MB_Frame_Width+j]=(i+1)*MB_Frame_Width+j;
                    if (j<MB_Frame_Width-1)
                    {
                        next_mb_tables[1][(i+1)*MB_Frame_Width+j]=i*MB_Frame_Width+j+1;
                    }
                    else
                    {
                        next_mb_tables[1][(i+1)*MB_Frame_Width+j]=(i+2)*MB_Frame_Width;
                    }
                }
            }

        }
        // Note that most of the slice data buffer will be usually unused,
        // as it is allocated for worst case. Only the first few entriies
        // (and frequently only the first) are typically used.

        // allocate macroblock info
        {
            size_t nMBCount = (desiredPaddedSize.width>>4) * (desiredPaddedSize.height>>4);

            // allocate buffer
            Ipp32s len = (Ipp32s) (sizeof(H264DecoderMacroblockMVs) +
                                   sizeof(H264DecoderMacroblockMVs) +
                                   sizeof(H264DecoderMacroblockMVFlags) +
                                   sizeof(H264DecoderMacroblockMVFlags) +
                                   sizeof(H264DecoderMacroblockCoeffsInfo) +
                                   sizeof(H264DecoderMacroblockLocalInfo)) * nMBCount + 16 * 6;

            m_pParsedDataNew = ippsMalloc_8u_x(len);
            if (NULL == m_pParsedDataNew)
                return UMC_FAILED_TO_ALLOCATE_BUFFER;
            ippsZero_8u_x(m_pParsedDataNew, len);

            // reset pointer(s)
            m_mbinfo.MVDeltas[0] = align_pointer<H264DecoderMacroblockMVs *> (m_pParsedDataNew, ALIGN_VALUE);
            m_mbinfo.MVDeltas[1] = align_pointer<H264DecoderMacroblockMVs *> (m_mbinfo.MVDeltas[0] + nMBCount, ALIGN_VALUE);
            m_mbinfo.MVFlags[0] = align_pointer<H264DecoderMacroblockMVFlags *> (m_mbinfo.MVDeltas[1] + nMBCount, ALIGN_VALUE);
            m_mbinfo.MVFlags[1] = align_pointer<H264DecoderMacroblockMVFlags *> (m_mbinfo.MVFlags[0] + nMBCount, ALIGN_VALUE);
            m_mbinfo.MacroblockCoeffsInfo = align_pointer<H264DecoderMacroblockCoeffsInfo *> (m_mbinfo.MVFlags[1] + nMBCount, ALIGN_VALUE);
            m_mbinfo.mbs = align_pointer<H264DecoderMacroblockLocalInfo *> (m_mbinfo.MacroblockCoeffsInfo + nMBCount, ALIGN_VALUE);
        }
    }

    return umcRes;
}

//////////////////////////////////////////////////////////////////////////////
// updateFrameNumWrap
//  Updates m_FrameNumWrap and m_PicNum if the frame is a short-term
//  reference and a frame number wrap has occurred.
//////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrame::UpdateFrameNumWrap(Ipp32s  CurrFrameNum, Ipp32s  MaxFrameNum,Ipp8u CurrPicStruct)
{
    if (isShortTermRef())
    {
        m_FrameNumWrap = m_FrameNum;
        if (m_FrameNum > CurrFrameNum)
            m_FrameNumWrap -= MaxFrameNum;
        //***bnie:if (CurrPicStruct>=FRM_STRUCTURE)
        {
            setPicNum(m_FrameNumWrap,0);
            m_PictureStructureForRef = FRM_STRUCTURE;
        }

    }

}    // updateFrameNumWrap

//////////////////////////////////////////////////////////////////////////////
// updateLongTermPicNum
//  Updates m_LongTermPicNum for if long term reference, based upon
//  m_LongTermFrameIdx.
//////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrame::UpdateLongTermPicNum(Ipp8u CurrPicStruct)
{
    if (isLongTermRef())
    {
        //***bnie: if (CurrPicStruct>=FRM_STRUCTURE)
        {
            m_LongTermPicNum[0] = m_LongTermFrameIdx;
            m_LongTermPicNum[1] = m_LongTermFrameIdx;
        }
    }
}    // updateLongTermPicNum

H264VideoDecoder::H264DecoderFrameList::H264DecoderFrameList(void)
{
    m_pHead = NULL;
    m_pTail = NULL;
    m_pCurrent = NULL;
    m_pFree = NULL;

} // H264VideoDecoder::H264DecoderFrameList::H264DecoderFrameList(void)

H264VideoDecoder::H264DecoderFrameList::~H264DecoderFrameList(void)
{
    Release();

} // H264VideoDecoder::H264DecoderFrameList::~H264DecoderFrameList(void)

void H264VideoDecoder::H264DecoderFrameList::Release(void)
{
    // destroy frame list
    while (m_pHead)
    {
        H264DecoderFrame *pNext = m_pHead->future();
        delete m_pHead;
        m_pHead = pNext;
    }
    // destroy free frames list
    while (m_pFree)
    {
        H264DecoderFrame *pNext = m_pFree->future();
        delete m_pFree;
        m_pFree = pNext;
    }

    m_pHead = NULL;
    m_pTail = NULL;
    m_pCurrent = NULL;
    m_pFree = NULL;

} // void H264VideoDecoder::H264DecoderFrameList::Release(void)

H264VideoDecoder::H264DecoderFrame*
H264VideoDecoder::H264DecoderFrameList::detachHead()
{
    H264DecoderFrame *pHead = m_pHead;
    if (pHead)
    {
        m_pHead = m_pHead->future();
        if (m_pHead)
            m_pHead->setPrevious(0);
        else
            m_pTail = 0;
    }
    return pHead;
}


//////////////////////////////////////////////////////////////////////////////
// append
//   Appends a new decoded frame buffer to the "end" of the linked list
//////////////////////////////////////////////////////////////////////////////
void
H264VideoDecoder::H264DecoderFrameList::append(H264DecoderFrame *pFrame)
{
    // Error check
    if (!pFrame)
    {
        // Sent in a NULL frame
        return;
    }

    // Has a list been constructed - is their a head?
    if (!m_pHead)
    {
        // Must be the first frame appended
        // Set the head to the current
        m_pHead = pFrame;
        m_pHead->setPrevious(0);
        m_pCurrent = m_pHead;
    }

    if (m_pTail)
    {
        // Set the old tail as the previous for the current
        pFrame->setPrevious(m_pTail);

        // Set the old tail's future to the current
        m_pTail->setFuture(pFrame);
    }
    else
    {
        // Must be the first frame appended
        // Set the tail to the current
        m_pTail = pFrame;
    }

    // The current is now the new tail
    m_pTail = pFrame;
    m_pTail->setFuture(0);

    //
}

void
H264VideoDecoder::H264DecoderFrameList::insertList(H264DecoderFrameList &src)
{
    if (!src.isEmpty())
    {
        src.tail()->setFuture(m_pHead);
        if (m_pHead)
            m_pHead->setPrevious(src.tail());
        m_pHead = src.head();

        if (!m_pTail)
            m_pTail = src.tail();

        src.m_pHead = src.m_pTail = 0;
    }
}


void
H264VideoDecoder::H264DecoderFrameList::insertAtCurrent(H264DecoderFrame *pFrame)
{
    if (m_pCurrent)
    {
        H264DecoderFrame *pFutureFrame = m_pCurrent->future();

        pFrame->setFuture(pFutureFrame);

        if (pFutureFrame)
        {
            pFutureFrame->setPrevious(pFrame);
        }
        else
        {
            // Must be at the tail
            m_pTail = pFrame;
        }

        pFrame->setPrevious(m_pCurrent);
        m_pCurrent->setFuture(pFrame);
    }
    else
    {
        // Must be the first frame
        append(pFrame);
    }
}


H264VideoDecoder::H264DecoderFrame *H264VideoDecoder::H264DecoderFrameList::findNextDisposable(void)
{/*
    if (NULL == m_pFree)
        return NULL;
    else
    {
        H264DecoderFrame *pReturn = m_pFree;

        m_pFree = m_pFree->future();
    }
*/
    H264DecoderFrame *pTmp;

    if (!m_pCurrent)
    {
        // There are no frames in the list, return
        return NULL;
    }

    // Loop through starting with the next frame after the current one
    for (pTmp = m_pCurrent->future(); pTmp; pTmp = pTmp->future())
    {
        if (pTmp->isDisposable())
        {
            // We got one

            // Update the current
            m_pCurrent = pTmp;

            return pTmp;
        }
    }

    // We got all the way to the tail without finding a free frame
    // Start from the head and go until the current
    for (pTmp = m_pHead; pTmp && pTmp->previous() != m_pCurrent; pTmp = pTmp->future())
    {
        if (pTmp->isDisposable())
        {
            // We got one

            // Update the current
            m_pCurrent = pTmp;
            return pTmp;
        }
    }

    // We never found one
    return NULL;

} // H264VideoDecoder::H264DecoderFrame *H264VideoDecoder::H264DecoderFrameList::findNextDisposable(void)

///////////////////////////////////////////////////////////////////////////////
// findOldestDisplayable
// Search through the list for the oldest displayable frame. It must be
// not disposable, not outputted, and have smallest PicOrderCnt.
///////////////////////////////////////////////////////////////////////////////
H264VideoDecoder::H264DecoderFrame *
H264VideoDecoder::H264DecoderFrameList::findOldestDisplayable()
{
    H264DecoderFrame *pCurr = m_pHead;
    H264DecoderFrame *pOldest = NULL;
    Ipp32s  SmallestPicOrderCnt = 0x7fffffff;    // very large positive
    Ipp32s  LargestRefPicListResetCount = 0;

    while (pCurr)
    {
        if (pCurr->isDisplayable() && !pCurr->wasOutputted())
        {
            // corresponding frame
            if (pCurr->RefPicListResetCount_kinoma_always_frame() > LargestRefPicListResetCount )
            {
                pOldest = pCurr;
                SmallestPicOrderCnt = pCurr->PicOrderCnt_kinoma_always_frame();
                LargestRefPicListResetCount = pCurr->RefPicListResetCount_kinoma_always_frame();
            }
            else if ((pCurr->PicOrderCnt_kinoma_always_frame() < SmallestPicOrderCnt ) &&
                     (pCurr->RefPicListResetCount_kinoma_always_frame() == LargestRefPicListResetCount ))
            {
                pOldest = pCurr;
                SmallestPicOrderCnt = pCurr->PicOrderCnt_kinoma_always_frame();
            }
        }
        pCurr = pCurr->future();
    }
    // may be OK if NULL
    return pOldest;

}    // findOldestDisplayable

///////////////////////////////////////////////////////////////////////////////
// countNumDisplayable
//  Return number of displayable frames.
///////////////////////////////////////////////////////////////////////////////
Ipp32s H264VideoDecoder::H264DecoderFrameList::countNumDisplayable()
{
    H264DecoderFrame *pCurr = m_pHead;
    Ipp32u NumDisplayable = 0;

    while (pCurr)
    {
        if (pCurr->isDisplayable() && !pCurr->wasOutputted())
            NumDisplayable++;
        pCurr = pCurr->future();
    }
    return NumDisplayable;
}    // countNumDisplayable

///////////////////////////////////////////////////////////////////////////////
// countActiveRefs
//  Return number of active short and long term reference frames.
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::countActiveRefs(Ipp32u &NumShortTerm, Ipp32u &NumLongTerm)
{
    H264DecoderFrame *pCurr = m_pHead;
    NumShortTerm = 0;
    NumLongTerm = 0;

    while (pCurr)
    {
        if (pCurr->isShortTermRef())
            NumShortTerm++;
        else if (pCurr->isLongTermRef())
            NumLongTerm++;
        pCurr = pCurr->future();
    }

}    // countActiveRefs

///////////////////////////////////////////////////////////////////////////////
// removeAllRef
// Marks all frames as not used as reference frames.
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::removeAllRef()
{
    H264DecoderFrame *pCurr = m_pHead;

    while (pCurr)
    {
        pCurr->unSetisLongTermRef_kinoma_always_frame();
        //***bnie: pCurr->unSetisLongTermRef_kinoma_always_frame();
        pCurr->unSetisShortTermRef_kinoma_always_frame();
        //***bnie: pCurr->unSetisShortTermRef_kinoma_always_frame();
        pCurr = pCurr->future();
    }

}    // removeAllRef

void H264VideoDecoder::H264DecoderFrameList::IncreaseRefPicListResetCount(H264DecoderFrame *ExcludeFrame)
{
    H264DecoderFrame *pCurr = m_pHead;

    while (pCurr)
    {
        if (pCurr!=ExcludeFrame)
        {
            pCurr->IncreaseRefPicListResetCount(0);
            pCurr->IncreaseRefPicListResetCount(1);
        }
        pCurr = pCurr->future();
    }

}    // IncreaseRefPicListResetCount

///////////////////////////////////////////////////////////////////////////////
// removeAllDisplayable
// Marks all frames as not displayable.
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::removeAllDisplayable()
{
    H264DecoderFrame *pCurr = m_pHead;

    while (pCurr)
    {
        pCurr->unSetisDisplayable();
        pCurr = pCurr->future();
    }

}    // removeAllDisplayable

///////////////////////////////////////////////////////////////////////////////
// freeOldestShortTermRef
// Marks the oldest (having smallest FrameNumWrap) short-term reference frame
// as not used as reference frame.
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::freeOldestShortTermRef()
{
    H264DecoderFrame *pCurr = m_pHead;
    H264DecoderFrame *pOldest = NULL;
    Ipp32s  SmallestFrameNumWrap = 0x0fffffff;    // very large positive

    while (pCurr)
    {
        if (pCurr->isShortTermRef() && (pCurr->FrameNumWrap() < SmallestFrameNumWrap))
        {
            pOldest = pCurr;
            SmallestFrameNumWrap = pCurr->FrameNumWrap();
        }
        pCurr = pCurr->future();
    }

    VM_ASSERT(pOldest != NULL);    // Should not have been called if no short-term refs

    if (pOldest)
    {
        pOldest->unSetisShortTermRef_kinoma_always_frame();
        //***bnie: pOldest->unSetisShortTermRef_kinoma_always_frame(1);
    }

}    // freeOldestShortTermRef

///////////////////////////////////////////////////////////////////////////////
// freeShortTermRef
// Mark the short-term reference frame with specified PicNum as not used
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::freeShortTermRef(Ipp32s  PicNum)
{
    H264DecoderFrame *pCurr = m_pHead;
    //bool found = false;
    while (pCurr)
    {
        //***bnie:if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if (pCurr->isShortTermRef() && (pCurr->PicNum(0) == PicNum))
            {
                pCurr->unSetisShortTermRef_kinoma_always_frame();
                break;
            }
        }


        pCurr = pCurr->future();
    }
    VM_ASSERT(pCurr != NULL);    // No match found, should not happen.

}    // freeShortTermRef

///////////////////////////////////////////////////////////////////////////////
// freeLongTermRef
// Mark the long-term reference frame with specified LongTermPicNum as not used
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::freeLongTermRef(Ipp32s  LongTermPicNum)
{
    H264DecoderFrame *pCurr = m_pHead;
    //bool found = false;

    while (pCurr)
    {
        //***bnie:if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if (pCurr->isLongTermRef() && (pCurr->LongTermPicNum(0) == LongTermPicNum))
            {
                pCurr->unSetisLongTermRef_kinoma_always_frame();
                break;
            }
        }

        pCurr = pCurr->future();
    }
    VM_ASSERT(pCurr != NULL);    // No match found, should not happen.

}    // freeLongTermRef

///////////////////////////////////////////////////////////////////////////////
// freeLongTermRef
// Mark the long-term reference frame with specified LongTermFrameIdx
// as not used
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::freeLongTermRefIdx(Ipp32s  LongTermFrameIdx)
{
    H264DecoderFrame *pCurr = m_pHead;
    //bool found = false;

    while (pCurr)
    {
        //***bnie:if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if (pCurr->isLongTermRef() && (pCurr->LongTermFrameIdx() == LongTermFrameIdx ))
            {
                pCurr->unSetisLongTermRef_kinoma_always_frame();
                break;
            }
        }

        pCurr = pCurr->future();
    }

    // OK if none found

}    // freeLongTermRefIdx

///////////////////////////////////////////////////////////////////////////////
// freeOldLongTermRef
// Mark any long-term reference frame with LongTermFrameIdx greater
// than MaxLongTermFrameIdx as not used. When MaxLongTermFrameIdx is -1, this
// indicates no long-term frame indices and all long-term reference
// frames should be freed.
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::freeOldLongTermRef(Ipp32s  MaxLongTermFrameIdx)
{
    H264DecoderFrame *pCurr = m_pHead;

    while (pCurr)
    {
        if (pCurr->isLongTermRef_kinoma_always_frame() && (pCurr->LongTermFrameIdx() > MaxLongTermFrameIdx))
        {
            pCurr->unSetisLongTermRef_kinoma_always_frame();
            //***bnie: pCurr->unSetisLongTermRef(1);
        }
        pCurr = pCurr->future();
    }

    // OK to not find any to free

}    // freeOldLongTermRef

///////////////////////////////////////////////////////////////////////////////
// changeSTtoLTRef
//    Mark the short-term reference frame with specified PicNum as long-term
//  with specified long term idx.
///////////////////////////////////////////////////////////////////////////////
void H264VideoDecoder::H264DecoderFrameList::changeSTtoLTRef(Ipp32s  PicNum, Ipp32s  LongTermFrameIdx)
{
    H264DecoderFrame *pCurr = m_pHead;
    //bool found=false;
    while (pCurr)
    {
        //***bnie:if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if (pCurr->isShortTermRef() && (pCurr->PicNum(0) == PicNum))
            {
                pCurr->unSetisShortTermRef_kinoma_always_frame();
                pCurr->setLongTermFrameIdx(LongTermFrameIdx);
                pCurr->SetisLongTermRef_kinoma_always_frame();
                pCurr->UpdateLongTermPicNum(2);
                break;
            }
        }

		pCurr = pCurr->future();
    }
    VM_ASSERT(pCurr != NULL);    // No match found, should not happen.

}    // changeSTtoLTRef


Status H264VideoDecoder::SetMBMap()
{
    Status status = UMC_OK;
    Ipp32u mbnum, i;
    Ipp32s prevMB;
    Ipp32u uNumMBCols;
    Ipp32u uNumMBRows;
    Ipp32u uNumSliceGroups;
    Ipp32u uNumMapUnits;
    H264PicParamSet *pps = &m_PicParamSet[m_FirstSliceHeader->pic_parameter_set_id];
    Ipp32s PrevMapUnit[MAX_NUM_SLICE_GROUPS];
    Ipp32s SliceGroup, FirstMB;
    Ipp8u *pMap = NULL;
    bool bSetFromMap = false;

    uNumMBCols = m_pCurrentFrame->macroBlockSize().width;
    uNumMBRows = m_pCurrentFrame->macroBlockSize().height;
    FirstMB = 0;
    // TBD: update for fields:
    uNumMapUnits = uNumMBCols*uNumMBRows;
    uNumSliceGroups = pps->num_slice_groups;

    // If number of slice groups is one, MB order is raster scan order
    if (uNumSliceGroups == 1)
    {/*
        memset(m_mbinfo.MacroblockCoeffsInfo+FirstMB,0,uNumMBRows*uNumMBCols*sizeof(H264DecoderMacroblockCoeffsInfo));*/
        {
            m_mbinfo.active_next_mb_table = next_mb_tables[0];
        }
        m_bNeedToCheckMBSliceEdges = false;
    }
#ifndef DROP_SLICE_GROUP
    else
    {/*
        //since main profile doesn't allow slice groups to be >1 and in baseline no fields (or mbaffs) allowed
        //the following memset is ok.
        memset(m_mbinfo.MacroblockCoeffsInfo+FirstMB,0,uNumMBRows*uNumMBCols*sizeof(H264DecoderMacroblockCoeffsInfo));*/
        // > 1 slice group
        m_bNeedToCheckMBSliceEdges = true;

        switch (pps->SliceGroupInfo.slice_group_map_type)
        {
        case 0:
            {
                // interleaved slice groups: run_length for each slice group,
                // repeated until all MB's are assigned to a slice group
                Ipp32u NumThisGroup;

                // Init PrevMapUnit to -1 (none), for first unit of each slice group
                for (i=0; i<uNumSliceGroups; i++)
                    PrevMapUnit[i] = -1;

                SliceGroup = 0;
                NumThisGroup = 0;
                prevMB = -1;
                for (mbnum=FirstMB; mbnum<uNumMapUnits; mbnum++)
                {
                    if (NumThisGroup == pps->SliceGroupInfo.run_length[SliceGroup])
                    {
                        // new slice group
                        PrevMapUnit[SliceGroup] = prevMB;
                        SliceGroup++;
                        if (SliceGroup == (Ipp32s)uNumSliceGroups)
                            SliceGroup = 0;
                        prevMB = PrevMapUnit[SliceGroup];
                        NumThisGroup = 0;
                    }
                    if (prevMB >= 0)
                    {
                        // new
                        next_mb_tables[2][prevMB] = mbnum;
                    }
                    prevMB = mbnum;
                    NumThisGroup++;
                }
            }
            m_mbinfo.active_next_mb_table = next_mb_tables[2];
            break;

        case 1:
            // dispersed
            {
                Ipp32u row, col;

                // Init PrevMapUnit to -1 (none), for first unit of each slice group
                for (i=0; i<uNumSliceGroups; i++)
                    PrevMapUnit[i] = -1;

                mbnum = FirstMB;
                for (row=0; row<uNumMBRows; row++)
                {
                    SliceGroup = ((row * uNumSliceGroups)/2) % uNumSliceGroups;
                    for (col=0; col<uNumMBCols; col++)
                    {
                        prevMB = PrevMapUnit[SliceGroup];
                        if (prevMB != -1)
                        {
                            next_mb_tables[2][prevMB]  = mbnum;
                        }
                        PrevMapUnit[SliceGroup] = mbnum;
                        mbnum++;
                        SliceGroup++;
                        if (SliceGroup == (Ipp32s)uNumSliceGroups)
                            SliceGroup = 0;
                    }    // col
                }    // row
            }
            m_mbinfo.active_next_mb_table = next_mb_tables[2];
            break;

        case 2:
            {
                // foreground + leftover: Slice groups are rectangles, any MB not
                // in a defined rectangle is in the leftover slice group, a MB within
                // more than one rectangle is in the lower-numbered slice group.

                // Two steps:
                // 1. Set m_pMBMap with slice group for all MBs.
                // 2. Set nextMB fields of MBInfo from m_pMBMap.

                Ipp32u RectUpper, RectLeft, RectRight, RectLower;
                Ipp32u RectRows, RectCols;
                Ipp32u row, col;

                // First init all as leftover
                for (mbnum=FirstMB; mbnum<uNumMapUnits; mbnum++)
                    m_pMBMap[mbnum] = (Ipp8u)(uNumSliceGroups - 1);

                // Next set those in slice group rectangles, from back to front
                for (SliceGroup=(Ipp32s)(uNumSliceGroups - 2); SliceGroup>=0; SliceGroup--)
                {
                    mbnum = pps->SliceGroupInfo.t1.top_left[SliceGroup];
                    RectUpper = pps->SliceGroupInfo.t1.top_left[SliceGroup] / uNumMBCols;
                    RectLeft = pps->SliceGroupInfo.t1.top_left[SliceGroup] % uNumMBCols;
                    RectLower = pps->SliceGroupInfo.t1.bottom_right[SliceGroup] / uNumMBCols;
                    RectRight = pps->SliceGroupInfo.t1.bottom_right[SliceGroup] % uNumMBCols;
                    RectRows = RectLower - RectUpper + 1;
                    RectCols = RectRight - RectLeft + 1;

                    for (row = 0; row < RectRows; row++)
                    {
                        for (col=0; col < RectCols; col++)
                        {
                            m_pMBMap[mbnum+col] = (Ipp8u)SliceGroup;
                        }    // col
                        mbnum += uNumMBCols;
                    }    // row
                }    // SliceGroup
            }
            m_mbinfo.active_next_mb_table = next_mb_tables[2];

            pMap = m_pMBMap;
            bSetFromMap = true;        // to cause step 2 to occur below
            break;
        case 3:
            {
                // Box-out, clockwise or counterclockwise. Result is two slice groups,
                // group 0 included by the box, group 1 excluded.

                // Two steps:
                // 1. Set m_pMBMap with slice group for all MBs.
                // 2. Set nextMB fields of MBInfo from m_pMBMap.

                Ipp32u x, y, leftBound, topBound, rightBound, bottomBound;
                Ipp32s xDir, yDir;
                Ipp32u mba;
                Ipp32u dir_flag = pps->SliceGroupInfo.t2.slice_group_change_direction_flag;
                Ipp32u uNumInGroup0;
                Ipp32u uGroup0Count = 0;

                SliceGroup = 1;        // excluded group

                uNumInGroup0 = pps->SliceGroupInfo.t2.slice_group_change_rate *
                                m_FirstSliceHeader->slice_group_change_cycle;
                if (uNumInGroup0 >= uNumMapUnits)
                {
                    // all units in group 0
                    uNumInGroup0 = uNumMapUnits;
                    SliceGroup = 0;
                    uGroup0Count = uNumInGroup0;    // to skip box out
                }

                // First init all
                for (mbnum=FirstMB; mbnum<uNumMapUnits; mbnum++)
                    m_pMBMap[mbnum] = (Ipp8u)SliceGroup;

                // Next the box-out algorithm to change included MBs to group 0

                // start at center
                x = (uNumMBCols - dir_flag)>>1;
                y = (uNumMBRows - dir_flag)>>1;
                leftBound = rightBound = x;
                topBound = bottomBound = y;
                xDir = dir_flag - 1;
                yDir = dir_flag;

                // expand out from center until group 0 includes the required number
                // of units
                while (uGroup0Count < uNumInGroup0)
                {
                    mba = x + y*uNumMBCols;
                    if (m_pMBMap[mba] == 1)
                    {
                        // add MB to group 0
                        m_pMBMap[mba] = 0;
                        uGroup0Count++;
                    }
                    if (x == leftBound && xDir == -1)
                    {
                        if (leftBound > 0)
                        {
                            leftBound--;
                            x--;
                        }
                        xDir = 0;
                        yDir = dir_flag*2 - 1;
                    }
                    else if (x == rightBound && xDir == 1)
                    {
                        if (rightBound < uNumMBCols - 1)
                        {
                            rightBound++;
                            x++;
                        }
                        xDir = 0;
                        yDir = 1 - dir_flag*2;
                    }
                    else if (y == topBound && yDir == -1)
                    {
                        if (topBound > 0)
                        {
                            topBound--;
                            y--;
                        }
                        xDir = 1 - dir_flag*2;
                        yDir = 0;
                    }
                    else if (y == bottomBound && yDir == 1)
                    {
                        if (bottomBound < uNumMBRows - 1)
                        {
                            bottomBound++;
                            y++;
                        }
                        xDir = dir_flag*2 - 1;
                        yDir = 0;
                    }
                    else
                    {
                        x += xDir;
                        y += yDir;
                    }
                }    // while
            }
            m_mbinfo.active_next_mb_table = next_mb_tables[2];

            pMap = m_pMBMap;
            bSetFromMap = true;        // to cause step 2 to occur below
            break;
        case 4:
            // raster-scan: 2 slice groups. Both groups contain units ordered
            // by raster-scan, so initializing nextMB for simple raster-scan
            // ordering is all that is required.
            m_mbinfo.active_next_mb_table = next_mb_tables[0];
/*            for (mbnum=FirstMB; mbnum<uNumMapUnits; mbnum++)
            {
                m_mbinfo.mbs[mbnum].NextMB = (Ipp16u) (mbnum + 1);
            }*/
            break;
        case 5:
            // wipe: 2 slice groups, the vertical version of case 4. Init
            // nextMB by processing the 2 groups as two rectangles (left
            // and right); to allow for the break between groups occurring
            // not at a column boundary, the rectangles also have an upper
            // and lower half (same heights both rectangles) that may vary
            // in width from one another by one macroblock, for example:
            //  L L L L R R R R R
            //  L L L L R R R R R
            //  L L L R R R R R R
            //  L L L R R R R R R
            {
                Ipp32u uNumInGroup0;
                Ipp32u uNumInLGroup;
                Ipp32s SGWidth;
                Ipp32s NumUpperRows;
                Ipp32s NumRows;
                Ipp32s row, col;
                Ipp32s iMBNum;

                uNumInGroup0 = pps->SliceGroupInfo.t2.slice_group_change_rate *
                                m_FirstSliceHeader->slice_group_change_cycle;
                if (uNumInGroup0 >= uNumMapUnits)
                {
                    // all units in group 0
                    uNumInGroup0 = uNumMapUnits;
                }
                if (pps->SliceGroupInfo.t2.slice_group_change_direction_flag == 0)
                    uNumInLGroup = uNumInGroup0;
                else
                    uNumInLGroup = uNumMapUnits - uNumInGroup0;

                if (uNumInLGroup > 0)
                {
                    // left group
                    NumUpperRows = uNumInLGroup % uNumMBRows;
                    NumRows = uNumMBRows;
                    SGWidth = uNumInLGroup / uNumMBRows;        // lower width, left
                    if (NumUpperRows)
                    {
                        SGWidth++;            // upper width, left

                        // zero-width lower case
                        if (SGWidth == 1)
                            NumRows = NumUpperRows;
                    }
                    iMBNum = FirstMB;

                    for (row = 0; row < NumRows; row++)
                    {
                        col = 0;
                        while (col < SGWidth-1)
                        {
                            next_mb_tables[2][iMBNum + col] = (iMBNum + col + 1);
                            col++;
                        }    // col

                        // next for last MB on row
                        next_mb_tables[2][iMBNum + col] = (iMBNum + uNumMBCols);
                        iMBNum += uNumMBCols;

                        // time to switch to lower?
                        NumUpperRows--;
                        if (NumUpperRows == 0)
                            SGWidth--;
                    }    // row
                }    // left group

                if (uNumInLGroup < uNumMapUnits)
                {
                    // right group
                    NumUpperRows = uNumInLGroup % uNumMBRows;
                    NumRows = uNumMBRows;
                    // lower width, right:
                    SGWidth = uNumMBCols - uNumInLGroup / uNumMBRows;
                    if (NumUpperRows)
                        SGWidth--;            // upper width, right
                    if (SGWidth > 0)
                    {
                        // first MB is on first row
                        iMBNum = uNumMBCols - SGWidth;
                    }
                    else
                    {
                        // zero-width upper case
                        SGWidth = 1;
                        iMBNum = (NumUpperRows + 1)*uNumMBCols - 1;
                        NumRows = uNumMBRows - NumUpperRows;
                        NumUpperRows = 0;
                    }

                    for (row = 0; row < NumRows; row++)
                    {
                        col = 0;
                        while (col < SGWidth-1)
                        {
                            next_mb_tables[2][iMBNum + col] = (iMBNum + col + 1);
                            col++;
                        }    // col

                        // next for last MB on row
                        next_mb_tables[2][iMBNum + col] = (iMBNum + uNumMBCols);

                        // time to switch to lower?
                        NumUpperRows--;
                        if (NumUpperRows == 0)
                        {
                            SGWidth++;
                            // fix next for last MB on row
                            next_mb_tables[2][iMBNum + col]= (iMBNum+uNumMBCols - 1);
                            iMBNum--;
                        }

                        iMBNum += uNumMBCols;

                    }    // row
                }    // right group
            }
            m_mbinfo.active_next_mb_table = next_mb_tables[2];
            break;
        case 6:
            // explicit map read from bitstream, contains slice group id for
            // each map unit
            m_mbinfo.active_next_mb_table = next_mb_tables[2];
            pMap = pps->SliceGroupInfo.t3.pSliceGroupIDMap;
            bSetFromMap = true;
            break;
        default:
            // can't happen
            VM_ASSERT(0);

        }    // switch map type

        if (bSetFromMap)
        {
            // Set the nextMB MBInfo field of a set of macroblocks depending upon
            // the slice group information in the map, to create an ordered
            // (raster-scan) linked list of MBs for each slice group. The first MB
            // of each group will be identified by the first slice header for each
            // group.

            // For each map unit get assigned slice group from the map
            // For all except the first unit in each
            // slice group, set the next field of the previous MB in that
            // slice group.

            // Init PrevMapUnit to -1 (none), for first unit of each slice group
            for (i=0; i<uNumSliceGroups; i++)
                PrevMapUnit[i] = -1;

            for (mbnum=FirstMB; mbnum<uNumMapUnits; mbnum++)
            {
                SliceGroup = pMap[mbnum];
                prevMB = PrevMapUnit[SliceGroup];
                if (prevMB != -1)
                {
                    next_mb_tables[2][prevMB] = mbnum;
                }
                PrevMapUnit[SliceGroup] = mbnum;
            }
        }
    }    // >1 slice group
#endif

    return status;

}    // setMBMap



#pragma pack(16)
#if 0
/* temporal class definition */
class H264DwordPointer_
{
public:
    // Default constructor
    H264DwordPointer_(void)
    {
        m_pDest = NULL;
        m_nByteNum = 0;
    }

    H264DwordPointer_ operator = (void *pDest)
    {
        m_pDest = (Ipp32u *) pDest;
        m_nByteNum = 0;
        m_iCur = 0;

        return *this;
    }

    // Increment operator
    H264DwordPointer_ &operator ++ (void)
    {
        if (4 == ++m_nByteNum)
        {
            *m_pDest = m_iCur;
            m_pDest += 1;
            m_nByteNum = 0;
            m_iCur = 0;
        }
        else
            m_iCur <<= 8;

        return *this;
    }

    Ipp8u operator = (Ipp8u nByte)
    {
        m_iCur = (m_iCur & ~0x0ff) | ((Ipp32u) nByte);

        return nByte;
    }

protected:
    Ipp32u *m_pDest;                                            // (Ipp32u *) pointer to destination buffer
    Ipp32u m_nByteNum;                                          // (Ipp32u) number of current byte in dword
    Ipp32u m_iCur;                                              // (Ipp32u) current dword
};

class H264SourcePointer_
{
public:
    // Default constructor
    H264SourcePointer_(void)
    {
        m_pSource = NULL;
    }

    H264SourcePointer_ &operator = (void *pSource)
    {
        m_pSource = (Ipp8u *) pSource;

        m_CurBytes[0] = m_pSource[0];
        m_CurBytes[1] = m_pSource[1];
        m_CurBytes[2] = m_pSource[2];
        m_pSource += 3;
        m_nZeros = 0;
        m_nRemovedBytes = 0;

        if (0 == m_CurBytes[1])
        {
            if (0 == m_CurBytes[0])
                m_nZeros = 2;
            else
                m_nZeros = 1;
        }

        return *this;
    }

    Ipp32u Increment(void)
    {
        // remove preventing byte
        if ((2 <= m_nZeros) &&
            (3 == m_CurBytes[2]))
        {
            m_nRemovedBytes += 1;

            // first byte is zero
            m_CurBytes[1] = m_pSource[0];
            m_CurBytes[2] = m_pSource[1];
            m_pSource += 2;

            if (0 == m_CurBytes[2])
            {
                if (m_CurBytes[1])
                    m_nZeros = 2;
                else
                    m_nZeros = 1;
            }
            else
                m_nZeros = 0;

            return 2;
        }
        else
        {
            // check current byte
            if (0 == m_CurBytes[2])
                m_nZeros += 1;
            else
                m_nZeros = 0;

            // shift bytes
            m_CurBytes[0] = m_CurBytes[1];
            m_CurBytes[1] = m_CurBytes[2];
            m_CurBytes[2] = m_pSource[0];

            m_pSource += 1;
            return 1;
        }

    }

    operator Ipp8u (void)
    {
        return m_CurBytes[0];
    }

    Ipp32u IncrementRemain(void)
    {
        // shift bytes
        m_CurBytes[0] = m_CurBytes[1];
        m_CurBytes[1] = m_CurBytes[2];
        m_CurBytes[2] = (Ipp8u) -1;

        return 1;
    }

    Ipp32u GetRemovedBytes(void)
    {
        return m_nRemovedBytes;
    }

protected:
    Ipp8u *m_pSource;                                           // (Ipp8u *) pointer to destination buffer
    Ipp8u m_CurBytes[4];                                        // (Ipp8u) current bytes in stream, one for alignment
    Ipp32u m_nZeros;                                            // (Ipp32u) number of preceding zeros
    Ipp32u m_nRemovedBytes;                                     // (Ipp32u) number of removed bytes
};

#pragma pack()

void SwapMemoryAndRemovePreventingBytes(void *pDestination, size_t &nDstSize, void *pSource, size_t nSrcSize)
{
    H264DwordPointer_ pDst;
    H264SourcePointer_ pSrc;
    size_t i;

    // DwordPointer object is swapping written bytes
    // H264SourcePointer_ removes preventing start-code bytes

    // reset pointer(s)
    pSrc = pSource;
    pDst = pDestination;

    // do swapping
    i = 0;
    while (i < (Ipp32u) nSrcSize - 2)
    {
        pDst = (Ipp8u) pSrc;
        i += pSrc.Increment();
        ++pDst;
    }

    // copy last 3 bytes
    while (i < (Ipp32u) nSrcSize)
    {
        pDst = (Ipp8u) pSrc;
        i += pSrc.IncrementRemain();
        ++pDst;
    }

    // write padding bytes
    nDstSize = nSrcSize - pSrc.GetRemovedBytes();
    while (nDstSize & 3)
    {
        pDst = (Ipp8u) (0);
        ++nDstSize;
        ++pDst;
    }

} // void SwapMemoryAndRemovePreventingBytes(void *pDst, size_t &nDstSize, void *pSrc, size_t nSrcSize)

int SwapInQTMediaSample(int spspps_only, int naluLengthSize, unsigned char *src, int size, void *m_void )
{
	MediaDataEx *m = (MediaDataEx  *)m_void;
	MediaDataEx::_MediaDataEx *ex = m->GetExData();
	unsigned char *dst = (unsigned char *)m->GetBufferPointer();
	int			  buf_size = m->GetBufferSize();
	int			  wanted_buf_size = 0;
	int			  s = 0;
	int			  d = 0;
    int			  i = 0;
 	
	if( ex == NULL )
		ex = (MediaDataEx::_MediaDataEx *)new MediaDataEx::_MediaDataEx(20);

	wanted_buf_size = size + ex->limit*3 + 128;
	if( wanted_buf_size > buf_size )
	{
		buf_size = wanted_buf_size;
		if( dst != NULL )
			ippsFree_x( dst );

		dst = ippsMalloc_8u_x(buf_size);
	}
 
	while( s <= size - 3 )
	{
		int				src_size = 0;
		int				dst_size = 0;
		unsigned char	nalu_type;
		int				is_valid_nalu;

		if( naluLengthSize == 4 )
			src_size = (src[s+0]<<24)|(src[s+1]<<16)|(src[s+2]<<8)|(src[s+3]);
		else if(  naluLengthSize == 2 )
			src_size = (src[s+0]<<8)|(src[s+1]<<0);
		else
			src_size = src[s+0];

		s += naluLengthSize;

		if( s + src_size > size || src_size <= 0 )	//safe guard frame boundary by checking slice boundary
			break;

		//only let in NALU that we can handle			bnie  --2/19/2009
		nalu_type = src[s] & NAL_UNITTYPE_BITS;
		is_valid_nalu;

		if( spspps_only )
			is_valid_nalu = (NAL_UT_SPS   == nalu_type )  || (NAL_UT_PPS       == nalu_type );
		else
			is_valid_nalu = (NAL_UT_SLICE == nalu_type )  || (NAL_UT_IDR_SLICE == nalu_type );

#if 0 //***bnie: 3/31/2009 inherit from intel code, no really necessary...
		if(src_size == 0)
			is_valid_nalu = 0;
		if (8 == src_size)
		{
			Ipp8u *src_buffer = src+s;
			// Need to access src.data as bytes, not as a couple of Ipp32u's,
			// because theoretically it might not be 4-byte aligned.
			// Check the later bytes first, since they're more likely to
			// be non-zero in the general case.
			if ((src_buffer[7] == 0) && (src_buffer[6] == 0) &&
				(src_buffer[5] == 0) && (src_buffer[4] == 0) &&
				(src_buffer[3] == 0) && (src_buffer[2] == 0) &&
				(src_buffer[1] == 0) && (src_buffer[0] == 0))
			{
				is_valid_nalu = 0;
			}
		}
#endif

		if ( is_valid_nalu )
		{
			unsigned char *this_src = src+s;
			unsigned char *this_dst = dst+d+4;
			int src_bit_offset = (((int)(this_src))&0x03)<<3;

			//***bnie ugly hack to make intel code happy with its unhealthy hobby about start code
			dst[d+0] = 1; dst[d+1] = 0; dst[d+2] = 0;dst[d+3] = 0;

			SwapMemoryAndRemovePreventingBytes(dst+d+4, (size_t &)dst_size, src+s, src_size);

			i++;
			if( i > 20 )
				return -1;
			d += dst_size+4;
		}

		s += src_size;
	}

    ex->count  = i;
	dst[d+0] = 0xff;
	dst[d+1] = 0xff;
	dst[d+2] = 0xff;
	dst[d+3] = 0xff;
	dst[d+4] = 0xff;
	dst[d+5] = 0xff;
	dst[d+6] = 0xff;
	dst[d+7] = 0xff;

	//printf("set buffer: %x\n", dst);
	m->SetBufferPointer((Ipp8u*)dst, d );//buf_size);
	//m->SetTime(0, 0);
	m->SetExData( ex );

	return 0;
}

#define SWAP(s)  s = (((s>>0)&0xff)<<24)|(((s>>8)&0xff)<<16)|(((s>>16)&0xff)<<8)|(((s>>24)&0xff)<<0)

int Delete_MediaDataEx(void *m_void )
{
	MediaDataEx *m = (MediaDataEx  *)m_void;
	MediaDataEx::_MediaDataEx *ex = m->GetExData();
    void   *buf = NULL;

    buf = m->GetBufferPointer();
	
	if( buf != NULL )
		ippsFree_x( buf );

	m->SetBufferPointer( NULL, 0 );

	delete 	ex;
	m->SetExData( NULL );

	return 0;
}
#endif

void H264VideoDecoder::ResetGMBData()
{
    Ipp32s i,j; //, firstj = 0;
    mb_width  = m_pCurrentFrame->macroBlockSize().width;
    mb_height = m_pCurrentFrame->macroBlockSize().height;
    //***bnie: Ipp8u mbaff = m_FirstSliceHeader->MbaffFrameFlag;
    H264DecoderMacroblockGlobalInfo *g_mb_first=m_pCurrentFrame->m_mbinfo.mbs;
    H264DecoderMacroblockGlobalInfo *g_mb;

    g_mb = g_mb_first;


    for (j = 0; j < mb_height; j++)
    {
        for (i = 0; i < mb_width; i++, g_mb++)
        {
            g_mb->slice_id = -1;
            g_mb->mb_aux_fields = 0;//***bniebnie: (Ipp8u) ((j&mbaff)<<1);
        }
    }
}

Status  H264VideoDecoder::Reset()
{
    Status umcRes = UMC_OK;

#ifdef USE_SEI
    m_FrameProcessingStage = NORMAL_FRAME_PROCESSING;
#endif

    //***m_local_frame_time         = 0;

    m_H264DecoderFramesList.removeAllRef();
    m_H264DecoderFramesList.removeAllDisplayable();

    m_bSeqParamSetRead  = true;
    m_bPicParamSetRead  = true;
    m_ReallySkipped     = 0;
    m_AskedSkipped      = 0;
    m_NeedToSkip        = 0;
    m_SkipRepeat        = 0;
    m_PermanentTurnOffDeblocking = 0;
    m_getframe_calls    = 0;
    m_bIsDecodingStarted= false;
    //***bnie: m_field_index_kinoma_always_zero       = 0;
    m_WaitForDR         = true;
    //***m_bHasSEI           = false;
    // m_SkipThisPic       = false;

    m_CurrentSeqParamSet = -1;
    m_CurrentPicParamSet = -1;

    /* DEBUG : skipping was turned OFF, require to reimplement
    m_SkipThisPic(false)
    m_PreviousPicSkipped(false)*/
    m_SkipFlag = 0;
    m_SkipCycle = 1;
    m_ModSkipCycle = 1;

    Ipp32s i;
    for (i=0; i<MAX_NUM_SEQ_PARAM_SETS; i++)
    {
        m_SeqParamSet[i].poffset_for_ref_frame = NULL;
        m_SeqParamSet[i].seq_parameter_set_id = MAX_NUM_SEQ_PARAM_SETS;    // illegal id
    }

    for (i=0; i<MAX_NUM_PIC_PARAM_SETS; i++)
    {
        m_PicParamSet[i].pFMOMap = NULL;
        m_PicParamSet[i].pic_parameter_set_id = MAX_NUM_PIC_PARAM_SETS;    // illegal id
    }

    //***bnie: delete m_pFrameLevel_BitStream;

    //***bnie: m_pFrameLevel_BitStream = new H264Bitstream();
    //***bnie: if (!m_pFrameLevel_BitStream)
    //***bnie:     umcRes = UMC_FAILED_TO_ALLOCATE_BUFFER;

    //***bnie: if (umcRes != UMC_OK)
    //***bnie: {
    //***bnie:     delete m_pFrameLevel_BitStream;
    //***bnie:     m_pFrameLevel_BitStream = NULL;
    //***bnie: }

    m_pDisplayFrame = 0;
    m_pCurrentFrame = 0;

    //Ipp32s num=4;
    //ChangeVideoDecodingSpeed(num);
    return umcRes;
}
} // end namespace UMC
