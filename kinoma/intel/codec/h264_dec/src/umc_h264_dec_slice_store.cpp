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

#include "umc_h264_dec_slice_store.h"
#include "umc_h264_slice_decoding.h"
#include "umc_h264_segment_decoder.h"
#include "umc_h264_segment_decoder_mt.h"
#include "umc_media_data_ex.h"
#include "umc_h264_bitstream.h"/* DEBUG: extra header */
#include "vm_mutex.h"
#include "vm_event.h"
#include "vm_semaphore.h"
#include "umc_automatic_mutex.h"

namespace UMC
{

enum
{
    MINIMUM_SLICE_LENGTH        = 8
};

enum
{
    CURRENT_SLICE,
    OTHER_SLICES
};

//#define ECHO
//#define ECHO_DEB


H264ThreadedDeblockingTools::H264ThreadedDeblockingTools(void)
{

} // H264ThreadedDeblockingTools::H264ThreadedDeblockingTools(void)

H264ThreadedDeblockingTools::~H264ThreadedDeblockingTools(void)
{
    Release();

} // H264ThreadedDeblockingTools::~H264ThreadedDeblockingTools(void)

void H264ThreadedDeblockingTools::Release(void)
{
    // there is nothing to do

} // void H264ThreadedDeblockingTools::Release(void)

bool H264ThreadedDeblockingTools::Init(Ipp32s iConsumerNumber)
{/*
    Ipp32s i;*/

    // release object before initialization
    Release();

    // save variables
    m_iConsumerNumber = min(NUMBER_OF_DEBLOCKERS, iConsumerNumber);

    // allocate working flags
    if (false == m_bThreadWorking.Init(iConsumerNumber))
        return false;

    // allocate array for current macroblock number
    if (false == m_iCurMBToDeb.Init(iConsumerNumber))
        return false;

    return true;

} // bool H264ThreadedDeblockingTools::Init(Ipp32s iConsumerNumber)

static
void GetDeblockingRange(Ipp32s &iLeft,
                        Ipp32s &iRight,
                        Ipp32s iMBWidth,
                        Ipp32s iThreadNumber,
                        Ipp32s iConsumerNumber,
                        Ipp32s iDebUnit)
{
    // calculate left value
    if (0 == iThreadNumber)
        iLeft = 0;
    else
        iLeft = align_value<Ipp32s> ((iMBWidth * iThreadNumber) / iConsumerNumber, iDebUnit);

    // calculate right value
    if (iConsumerNumber - 1 == iThreadNumber)
        iRight = iMBWidth - iDebUnit;
    else
        // calculate right as left_for_next_thread - deb_unit
        iRight = align_value<Ipp32s> ((iMBWidth * (iThreadNumber + 1)) / iConsumerNumber, iDebUnit) - iDebUnit;

} // void GetDeblockingRange(Ipp32s &iLeft,

void H264ThreadedDeblockingTools::Reset(Ipp32s iFirstMB, Ipp32s iMaxMB, Ipp32s iDebUnit, Ipp32s iMBWidth)
{
    Ipp32s i;
    Ipp32s iCurrMB = iFirstMB;
    Ipp32s iCurrMBInRow = iCurrMB % iMBWidth;

    // save variables
    m_iMaxMB = iMaxMB;
    m_iDebUnit = iDebUnit;
    m_iMBWidth = iMBWidth;

    // reset event(s) & current position(s)
    for (i = 0; i < m_iConsumerNumber; i += 1)
    {
        Ipp32s iLeft, iRight;

        // get working range for this thread
        GetDeblockingRange(iLeft, iRight, iMBWidth, i, m_iConsumerNumber, iDebUnit);

        // reset current MB to deblock
        if (iCurrMBInRow < iLeft)
            m_iCurMBToDeb[i] = iCurrMB - iCurrMBInRow + iLeft;
        else if (iCurrMBInRow > iRight)
            m_iCurMBToDeb[i] = iCurrMB - iCurrMBInRow + iMBWidth + iLeft;
        else
            m_iCurMBToDeb[i] = iCurrMB;

        // set current thread working status
        m_bThreadWorking[i] = false;
    }

} // void H264ThreadedDeblockingTools::Reset(Ipp32s iFirstMB, Ipp32s iMaxMB, Ipp32s iDebUnit, Ipp32s iMBWidth)

bool H264ThreadedDeblockingTools::GetMBToProcess(H264Task *pTask)
{
    Ipp32s iThreadNumber;
    Ipp32s iFirstMB, iMBToProcess;

    for (iThreadNumber = 0; iThreadNumber < m_iConsumerNumber; iThreadNumber += 1)
    {
        if (GetMB(iThreadNumber, iFirstMB, iMBToProcess))
        {
            // prepare task
            pTask->m_bDone = false;
            pTask->m_iFirstMB = iFirstMB;
            pTask->m_iMaxMB = m_iMaxMB;
            pTask->m_iMBToProcess = iMBToProcess;
            pTask->m_iTaskID = TASK_DEB_THREADED;
            pTask->m_pBuffer = NULL;
            pTask->m_pBufferEnd = NULL;
            pTask->m_pSlice = NULL;
 #ifndef DROP_MULTI_THREAD
            pTask->pFunctionMulti = &H264SegmentDecoderMultiThreaded::DeblockSegment;
#endif
			pTask->pFunctionSingle = NULL;

#ifdef ECHO_DEB
            {
                char cStr[256];
                sprintf(cStr, "(%d) dbt - % 4d to % 4d\n",
                    pTask->m_iThreadNumber,
                    pTask->m_iFirstMB,
                    pTask->m_iFirstMB + pTask->m_iMBToProcess);
                OutputDebugString(cStr);
            }
#endif // ECHO_DEB

            return true;
        }
    }

    return false;

} // bool H264ThreadedDeblockingTools::GetMBToProcess(H264Task *pTask)

bool H264ThreadedDeblockingTools::GetMB(Ipp32s iThreadNumber,
                                        Ipp32s &iFirstMB,
                                        Ipp32s &iMBToProcess)
{
    Ipp32s iCur;
    Ipp32s iNumber = iThreadNumber;
    Ipp32s iLeft, iRight;

    // do we have anything to do ?
    iCur = m_iCurMBToDeb[iNumber];
    if ((iCur >= m_iMaxMB) ||
        (m_bThreadWorking[iThreadNumber]))
        return false;

    // get working range for this thread
    GetDeblockingRange(iLeft, iRight, m_iMBWidth, iNumber, m_iConsumerNumber, m_iDebUnit);

    // wait left border macroblock
    if ((iNumber) &&
        (iLeft == (iCur % m_iMBWidth)))
    {
        if (m_iCurMBToDeb[iNumber - 1] < iCur)
            return false;

        iFirstMB = m_iCurMBToDeb[iNumber];
        iMBToProcess = m_iDebUnit;
        m_bThreadWorking[iThreadNumber] = true;

        return true;
    }

    // wait right border macroblock
    if ((iNumber != m_iConsumerNumber - 1) &&
        (iRight == (iCur % m_iMBWidth)))
    {
        if (m_iCurMBToDeb[iNumber + 1] <= iCur + m_iDebUnit - m_iMBWidth)
            return false;

        iFirstMB = m_iCurMBToDeb[iNumber];
        iMBToProcess = m_iDebUnit;
        m_bThreadWorking[iThreadNumber] = true;

        return true;
    }

    // deblock macroblocks between borders
    {
        iFirstMB = m_iCurMBToDeb[iNumber];
        if (iNumber != m_iConsumerNumber - 1)
        {
            iMBToProcess = min(m_iMaxMB - m_iCurMBToDeb[iNumber],
                               iRight - (m_iCurMBToDeb[iNumber] % m_iMBWidth));
        }
        else
        {
            iMBToProcess = min(m_iMaxMB - m_iCurMBToDeb[iNumber],
                               iRight + m_iDebUnit - (m_iCurMBToDeb[iNumber] % m_iMBWidth));
        }
        m_bThreadWorking[iThreadNumber] = true;

        return true;
    }

} // bool H264ThreadedDeblockingTools::GetMB(Ipp32s iThreadNumber,

void H264ThreadedDeblockingTools::SetProcessedMB(H264Task *pTask)
{
    Ipp32s iThreadNumber;

    for (iThreadNumber = 0; iThreadNumber < m_iConsumerNumber; iThreadNumber += 1)
    {
        if (m_iCurMBToDeb[iThreadNumber] == pTask->m_iFirstMB)
        {
            SetMB(iThreadNumber, pTask->m_iFirstMB, pTask->m_iMBToProcess);
            break;
        }
    }

} // void H264ThreadedDeblockingTools::SetProcessedMB(H264Task *pTask)

void H264ThreadedDeblockingTools::SetMB(Ipp32s iThreadNumber,
                                        Ipp32s iFirstMB,
                                        Ipp32s iMBToProcess)
{
    Ipp32s iLeft, iRight, iCur;
    Ipp32s iNumber;

    // fill variables
    iNumber = iThreadNumber;
    iCur = iFirstMB;

    // get working range for this thread
    GetDeblockingRange(iLeft, iRight, m_iMBWidth, iNumber, m_iConsumerNumber, m_iDebUnit);

    // increment current working MB index
    iCur += iMBToProcess;

    // correct current macroblock index to working range
    {
        Ipp32s iRest = iCur % m_iMBWidth;

        if (iRest > iRight)
            iCur = iCur - iRest + m_iMBWidth + iLeft;
        else if (iRest == 0)
            iCur = iCur + iLeft;

        // save current working MB index
        m_iCurMBToDeb[iNumber] = iCur;
    }

    m_bThreadWorking[iNumber] = false;

} // void H264ThreadedDeblockingTools::SetMB(Ipp32s iThreadNumber,

bool H264ThreadedDeblockingTools::IsDeblockingDone(void)
{
    bool bDeblocked = false;
    Ipp32s i;

    // test whole slice deblocking condition
    for (i = 0; i < m_iConsumerNumber; i += 1)
    {
        if (m_iCurMBToDeb[i] < m_iMaxMB)
            break;
    }
    if (m_iConsumerNumber == i)
        bDeblocked = true;

    return bDeblocked;

} // bool H264ThreadedDeblockingTools::IsDeblockingDone(void)

H264Slice::H264Slice(void)
{
    // DEBUG tools: set following variable to turn of deblocking
    m_bPermanentTurnOffDeblocking = false;
    // end of DEBUG tools

    m_pPicParamSet = NULL;
    m_pSeqParamSet = NULL;

    m_iMBWidth = -1;
    m_iMBHeight = -1;
    m_CurrentPicParamSet = -1;
    m_CurrentSeqParamSet = -1;

    m_pAllocatedBuffer = NULL;
    m_iAllocatedMB = 0;

    memset(m_pDecBuffer, 0, sizeof(m_pDecBuffer));
    memset(m_pRecBuffer, 0, sizeof(m_pRecBuffer));
    m_pAddBuffer = NULL;

    m_pNext = NULL;
    m_pCurrentFrame = 0;

	m_AdaptiveMarkingInfo_shadow = NULL;	//***bnie: 4/1/09

} // H264Slice::H264Slice(void)

H264Slice::~H264Slice(void)
{
    Release();

    if (m_pAllocatedBuffer)
        delete [] m_pAllocatedBuffer;
    m_pAllocatedBuffer = NULL;
    m_iAllocatedMB = 0;

} // H264Slice::~H264Slice(void)

void H264Slice::Release(void)
{
    m_pPicParamSet = NULL;
    m_pSeqParamSet = NULL;

    m_iMBWidth = -1;
    m_iMBHeight = -1;
    m_CurrentPicParamSet = -1;
    m_CurrentSeqParamSet = -1;
	m_AdaptiveMarkingInfo_shadow = NULL;	//***bnie: 4/1/09

    while (m_pAddBuffer)
    {
        H264MemoryPiece *pMem = m_pAddBuffer->GetNext();
        delete m_pAddBuffer;
        m_pAddBuffer = pMem;
    }

    m_pNext = NULL;

} // void H264Slice::Release(void)

bool H264Slice::Init(Ipp32s iConsumerNumber)
{
    // release object before initialization
    Release();

    // initialize threading tools
    if (1 < iConsumerNumber)
        m_DebTools.Init(iConsumerNumber);

    return true;

} // bool H264Slice::Init(Ipp32s iConsumerNumber)

bool H264Slice::SetSourceData(Ipp8u const *pSource, Ipp32s const bit_offset, Ipp32s const nSourceSize, Ipp32s const iNumber)
{
    Ipp32s iMBInFrame;
    //***bnie: Ipp32s iFieldIndex;

    // reset bit stream
    m_BitStream.Reset(pSource, bit_offset, nSourceSize);
    // set slice number
    SetSliceNumber(iNumber);

    // decode slice header
    if (false == DecodeSliceHeader())
        return false;


    iMBInFrame = (m_iMBWidth * m_iMBHeight);//***bnie: / ((m_SliceHeader.field_pic_flag) ? (2) : (1));

    // set slice variables
    m_iFirstMB = m_SliceHeader.first_mb_in_slice;//***bnie: + iMBInFrame * iFieldIndex;
    m_iMaxMB = iMBInFrame; //***bnie:  * (iFieldIndex + 1);
    m_bUnknownSize = IsSliceGroups();
    m_iAvailableMB = iMBInFrame;

    // reset all internal variables
    m_iCurMBToDec = m_iFirstMB;
    m_iCurMBToRec = m_iFirstMB;
    m_iCurMBToDeb = m_iFirstMB;

    m_bInProcess = false;
    m_bDecVacant = true;
    m_bRecVacant = true;
    m_bDebVacant = true;
    m_bFirstDebThreadedCall = true;

    // reallocate internal buffer
    if (false == m_bUnknownSize)
    {
        Ipp32s iMBRowSize = GetMBRowWidth();

        if ((NULL == m_pAllocatedBuffer) ||
            (m_iAllocatedMB < iMBRowSize * NUMBER_OF_ROWS))
        {
            Ipp32s i;

            if (m_pAllocatedBuffer)
                delete [] m_pAllocatedBuffer;
            m_pAllocatedBuffer = NULL;

            m_iAllocatedMB = iMBRowSize * NUMBER_OF_ROWS;
            m_pAllocatedBuffer = new Ipp16s[(iMBRowSize * COEFFICIENTS_BUFFER_SIZE + DEFAULT_ALIGN_VALUE) * NUMBER_OF_ROWS];
            if (NULL == m_pAllocatedBuffer)
                return false;

            for (i = 0; i < NUMBER_OF_ROWS; i += 1)
            {
                m_pDecBuffer[i] = align_pointer<Ipp16s *> (m_pAllocatedBuffer + (iMBRowSize * COEFFICIENTS_BUFFER_SIZE + DEFAULT_ALIGN_VALUE) * i, DEFAULT_ALIGN_VALUE);
                m_pRecBuffer[i] = NULL;
            }
        }
    }

    // recover from previous error(s)
    if (m_pRecBuffer[0])
    {
        Ipp32s i, j;

        i = 0;
        while (m_pDecBuffer[i])
            i++;

        j = 0;
        while (m_pRecBuffer[j])
        {
            m_pDecBuffer[i] = m_pRecBuffer[j];
            m_pRecBuffer[j] = NULL;
            i += 1;
            j += 1;
        }
    }

    // reset through-decoding variables
    m_nMBSkipCount = 0;
    m_nPassFDFDecode = 0;
    m_nQuantPrev = m_pPicParamSet[m_CurrentPicParamSet].pic_init_qp + m_SliceHeader.slice_qp_delta;
    m_prev_dquant = 0;
    //***bnie: m_field_index = iFieldIndex;
    m_bSkipNextFDF = false;
    if (IsSliceGroups())
        m_bNeedToCheckMBSliceEdges = true;
    else
        m_bNeedToCheckMBSliceEdges = (0 == m_SliceHeader.first_mb_in_slice) ? (false) : (true);

    // set conditional flags
    m_bDecoded = false;
    m_bPrevDeblocked = false;
    m_bDeblocked = false;

    // frame is not associated yet
    m_pCurrentFrame = NULL;

    return true;

} // bool H264Slice::Reset(void *pSource, size_t nSourceSize, Ipp32s iNumber)

/*
bool H264Slice::ResetLimited(void *pSource, size_t nSourceSize)
{
    // reset bit stream
    m_BitStream.Reset((Ipp8u *) pSource, (Ipp32u) nSourceSize);

    // decode slice header
    if (false == DecodeSliceHeader(false))
        return false;

    return true;

} // bool H264Slice::ResetLimited(void *pSource, size_t nSourceSize)
*/

void H264Slice::SetDestinationFrame(H264VideoDecoder::H264DecoderFrame *pCurrentFrame, H264DecoderLocalMacroblockDescriptor &mbinfo)
{
    m_pCurrentFrame = pCurrentFrame;
    m_mbinfo = mbinfo;

} // void H264Slice::SetDestinationFrame(H264VideoDecoder::H264DecoderFrame *pCurrentFrame, H264DecoderLocalMacroblockDescriptor &mbinfo)

void H264Slice::SetSliceNumber(Ipp32s iSliceNumber)
{
    m_iNumber = iSliceNumber;

} // void H264Slice::SetSliceNumber(Ipp32s iSliceNumber)

void H264Slice::UpdateReferenceList(H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList)
{
    if (true == m_bNeedToUpdateRefPicList)
    {
        UpdateRefPicList(pDecoderFrameList);
        m_bNeedToUpdateRefPicList = false;
        m_bReferenceReady = true;
    }

} // void H264Slice::UpdateReferenceList(H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList)

bool H264Slice::DecodeSliceHeader(bool bFullInitialization)
{
    Status umcRes = UMC_OK;
    // Locals for additional slice data to be read into, the data
    // was read and saved from the first slice header of the picture,
    // is not supposed to change within the picture, so can be
    // discarded when read again here.
    AdaptiveMarkingInfo AdaptiveMarkingInfoTemp0;
    AdaptiveMarkingInfo *pAdaptiveMarkingInfoTemp;
    bool bIsIDRPic = false;
    NAL_Unit_Type NALUType;
    Ipp8u nal_ref_idc;
    Ipp32s iSQUANT;

	if( m_AdaptiveMarkingInfo_shadow != NULL )
		pAdaptiveMarkingInfoTemp = m_AdaptiveMarkingInfo_shadow;
	else
		pAdaptiveMarkingInfoTemp = &AdaptiveMarkingInfoTemp0;


    memset(&m_SliceHeader, 0, sizeof(m_SliceHeader));

    while (UMC_OK == umcRes)
    {
        umcRes = m_BitStream.GetNALUnitType(NALUType, nal_ref_idc);
        if (UMC_OK != umcRes)
            return false;

        m_SliceHeader.nal_unit_type = NALUType;
        bIsIDRPic = (NALUType == NAL_UT_IDR_SLICE);
        // decode first part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart1(&m_SliceHeader);
        if (UMC_OK != umcRes)
            return false;

        m_CurrentPicParamSet = m_SliceHeader.pic_parameter_set_id;
        m_CurrentSeqParamSet = m_pPicParamSet[m_CurrentPicParamSet].seq_parameter_set_id;

        // decode second part of slice header
        umcRes = m_BitStream.GetSliceHeaderPart2(&m_SliceHeader,
                                                 m_PredWeight[0],
                                                 m_PredWeight[1],
                                                 &m_ReorderInfoL0,
                                                 &m_ReorderInfoL1,
                                                 pAdaptiveMarkingInfoTemp,
                                                 m_pPicParamSet + m_CurrentPicParamSet,
                                                 bIsIDRPic,
                                                 m_pSeqParamSet + m_CurrentSeqParamSet,
                                                 nal_ref_idc,
                                                 false);
        if (UMC_OK != umcRes)
            return false;

        // when we require only slice header
        if (false == bFullInitialization)
            return true;

        m_iMBWidth = m_pSeqParamSet[m_CurrentSeqParamSet].frame_width_in_mbs;
        m_iMBHeight = m_pSeqParamSet[m_CurrentSeqParamSet].frame_height_in_mbs;

        if (m_bPermanentTurnOffDeblocking)
            m_SliceHeader.disable_deblocking_filter_idc = DEBLOCK_FILTER_OFF;

        // redundant slice, discard
        if (m_SliceHeader.redundant_pic_cnt)
            continue;

        // Set next MB.
        if (m_SliceHeader.first_mb_in_slice >= (Ipp32s) (m_iMBWidth * m_iMBHeight))
        {
            return false;
        }

        iSQUANT = m_pPicParamSet[m_CurrentPicParamSet].pic_init_qp +
                  m_SliceHeader.slice_qp_delta;
        if (iSQUANT < QP_MIN || iSQUANT > QP_MAX)
        {
            return false;
        }

        // this flag would set to signal state
        // when slice will moved to main queue
        m_bNeedToUpdateRefPicList = false;
        m_bReferenceReady = false;

        // we sucessfully decoded slice header
        break;
    }

#ifndef DROP_CABAC
	if ((UMC_OK == umcRes) &&
        (m_pPicParamSet[m_CurrentPicParamSet].entropy_coding_mode))
    {
        // reset CABAC engine
        m_BitStream.InitializeDecodingEngine_CABAC();
        if (INTRASLICE == m_SliceHeader.slice_type)
        {
            m_BitStream.InitializeContextVariablesIntra_CABAC(m_pPicParamSet[m_CurrentPicParamSet].pic_init_qp +
                                                              m_SliceHeader.slice_qp_delta);
        }
        else
        {
            m_BitStream.InitializeContextVariablesInter_CABAC(m_pPicParamSet[m_CurrentPicParamSet].pic_init_qp +
                                                              m_SliceHeader.slice_qp_delta,
                                                              m_SliceHeader.cabac_init_idc);
        }
    }
#else
	if( m_pPicParamSet[m_CurrentPicParamSet].entropy_coding_mode )
		return UMC_UNSUPPORTED;
#endif

    return (UMC_OK == umcRes);

} // bool H264Slice::DecodeSliceHeader(bool bFullInitialization)

bool H264Slice::GetDeblockingCondition(void)
{
	// there is no deblocking
    if (DEBLOCK_FILTER_OFF == m_SliceHeader.disable_deblocking_filter_idc)
        return false;

    // no filtering edges of this slice
    if ((DEBLOCK_FILTER_ON_NO_SLICE_EDGES == m_SliceHeader.disable_deblocking_filter_idc) ||
        (m_bPrevDeblocked))
    {
        if (false == IsSliceGroups())
            return true;
    }

    return false;

} // bool H264Slice::GetDeblockingCondition(void)

enum
{
    DEFAULT_ITEM_NUMBER         = 8

};

H264SliceStore_::H264SliceStore_(H264PicParamSet *pPicParamSet, H264SeqParamSet *pSeqParamSet, H264VideoDecoder::H264DecoderFrameList *pDecoderFrameList) :
    m_pPicParamSet(pPicParamSet),
    m_pSeqParamSet(pSeqParamSet),
    m_pDecoderFrameList(pDecoderFrameList)
{
    m_pHeaderBuffer = 0;
    m_nHeaderBufferSize = 0;

    m_iSlicesInMainQueue = 0;
    m_iSlicesInAdditionalQueue = 0;
    m_pFreeSlices = NULL;

    m_iConsumerNumber = 0;

    m_pFilledMemory = NULL;
    m_pFreeMemory = NULL;
    m_pFreeBuffers = NULL;

    m_pMBIntraTypes[0] =
    m_pMBIntraTypes[1] = NULL;
    m_iMBIntraSizes[0] =
    m_iMBIntraSizes[1] = 0;

    //***m_MediaDataEx666.SetExData(&m_MediaDataEx_666);
#ifndef DROP_MULTI_THREAD
    vm_mutex_set_invalid(&m_mGuard);
    m_nWaitingThreads = 0;
#endif

} // H264SliceStore_::H264SliceStore_(H264PicParamSet *pPicParamSet, H264SeqParamSet *pSeqParamSet)

H264SliceStore_::~H264SliceStore_(void)
{
    Release();
#ifndef DROP_MULTI_THREAD
    if (vm_mutex_is_valid(&m_mGuard))
        vm_mutex_destroy(&m_mGuard);
#endif

} // H264SliceStore_::~H264SliceStore_(void)

void H264SliceStore_::Release(void)
{
#ifndef DROP_MULTI_THREAD
    Ipp32s i;
#endif

    // Waiting threads are not allowed in this function
    VM_ASSERT(0 == m_nWaitingThreads);

#ifndef DROP_MULTI_THREAD
    for (i = 0; i < m_iConsumerNumber; i += 1)
    {
        Event *pEvent = m_eWaiting[i];

        if (pEvent)
            pEvent->Reset();
    }
    m_nWaitingThreads = 0;
#endif

    // delete ready to decode slice(s)
    m_pMainQueue.Reset();

    // delete additional queue of slice(s)
    m_pAdditionalQueue.Reset();

    // delete free slice(s) list
    while (m_pFreeSlices)
    {
        H264Slice *pTemp = m_pFreeSlices->m_pNext;
        delete m_pFreeSlices;
        m_pFreeSlices = pTemp;
    }

    // delete allocated memory pieces
    while (m_pFreeMemory)
    {
        H264MemoryPiece *pMem = m_pFreeMemory->m_pNext;
        delete m_pFreeMemory;
        m_pFreeMemory = pMem;
    }
    while (m_pFilledMemory)
    {
        H264MemoryPiece *pMem = m_pFilledMemory->m_pNext;
        delete m_pFilledMemory;
        m_pFilledMemory = pMem;
    }

    while (m_pFreeBuffers)
    {
        H264MemoryPiece *pMem = m_pFreeBuffers->m_pNext;
        delete m_pFreeBuffers;
        m_pFreeBuffers = pMem;
    }

    if (m_pMBIntraTypes[0])
        ippsFree_x(m_pMBIntraTypes[0]);
    if (m_pMBIntraTypes[1])
        ippsFree_x(m_pMBIntraTypes[1]);

    m_pMBIntraTypes[0] =
    m_pMBIntraTypes[1] = NULL;
    m_iMBIntraSizes[0] =
    m_iMBIntraSizes[1] = 0;

    m_bDoFrameParallelization = false;

    ippsFree_x(m_pHeaderBuffer);
    m_nHeaderBufferSize = 0;

} // void H264SliceStore_::Release(void)

bool H264SliceStore_::Init(Ipp32s iConsumerNumber)
{
#ifndef DROP_MULTI_THREAD
    Ipp32s i;
#endif

    // release object before initialization
    Release();

    // we keep this variable due some optimizations purposes
    m_iConsumerNumber = iConsumerNumber;

#ifndef DROP_MULTI_THREAD
    // initialize threading tools
    if (0 == vm_mutex_is_valid(&m_mGuard))
    {
        if (VM_OK != vm_mutex_init(&m_mGuard))
            return false;
    }

    // initilaize event(s)
    for (i = 0; i < iConsumerNumber; i += 1)
    {
        if (NULL == m_eWaiting[i])
        {
            if (false == m_eWaiting.AllocateOneMore())
                return false;
            if (UMC_OK != m_eWaiting[i]->Init(0, 0))
                return false;
        }
    }
#endif

    // allocate default number of slices
    {
        Ipp32s j;

        for (j = 0; j < DEFAULT_ITEM_NUMBER; j += 1)
        {
            if (false == AllocateSlice())
                return false;
        }
    }

    // allocate arrays of pointers
    m_pMainQueue.Init(DEFAULT_ITEM_NUMBER);
    m_pAdditionalQueue.Init(DEFAULT_ITEM_NUMBER);

    // reset frame counters
    m_iProcessedFrames = 0;
    m_iAddedFrames = 0;
    m_iSlicesInLastPic = 0;

    m_bWaitForIDR = true;

    // initialize threaded deblocking tools
    if (1 < iConsumerNumber)
        m_DebTools.Init(iConsumerNumber);

    return true;

} // bool H264SliceStore_::Init(Ipp32s iConsumerNumber)

#if 0
static
Ipp32s FindStartCode(Ipp8u * (&pb), size_t &nSize)
{
    // there is no data
    if ((signed) nSize < 4)
        return 0;

    // find start code
    while ((4 <= nSize) && ((0 != pb[0]) ||
                            (0 != pb[1]) ||
                            (1 != pb[2])))
    {
        pb += 1;
        nSize -= 1;
    }

    if (4 <= nSize)
        return ((pb[0] << 24) | (pb[1] << 16) | (pb[2] << 8) | (pb[3]));

    return 0;

} // Ipp32s FindStartCode(Ipp8u * (&pb), size_t &nSize)
#endif

#pragma pack(16)
#if 0
class ippiBitStream
{
public:
    // Constructor
    ippiBitStream(Ipp8u *pb, Ipp32s iOffset) :
        m_p(pb),
        m_iOffset(iOffset)
    {

    }

    Ipp8u operator [] (int iIndex)
    {
        Ipp8u *p;

        // adjust index
        iIndex += 3 - ((m_iOffset + 1) / 8);

        // access required quadword
        p = m_p + (iIndex & -4);
        // return required byte
        return p[3 - iIndex & 3];
    }

    void operator += (int iAdd)
    {
        m_p += iAdd & -4;
        m_iOffset -= (iAdd & 3) * 8;
        // start next quad word
        if (0 > m_iOffset)
        {
            m_iOffset += 32;
            m_p += 4;
        }
    }

protected:
    Ipp8u *(&m_p);                                              // (Ipp8u * (&)) pointer to quadwords
    Ipp32s &m_iOffset;                                          // (Ipp32s &) offset bits in quadword

private:
    // lock assignment operator to avoid any accasionaly assignments
    ippiBitStream & operator = (ippiBitStream &)
    {
        return *this;
    }
};
#endif
#pragma pack()

//***bnie: we made sure only allowed NALU is parsed and processed at the very beginning
//			no need to recheck it here!!!

void H264SliceStore_::AddFilledMemoryPiece(H264MemoryPiece * (&pList), H264MemoryPiece *pMem)
{
    if (pList)
    {
        H264MemoryPiece *pTemp = pList;

        // find end of list
        while (pTemp->m_pNext)
            pTemp = pTemp->m_pNext;
        pTemp->m_pNext = pMem;
    }
    else
        pList = pMem;

    pMem->m_pNext = NULL;

} // void H264SliceStore_::AddFilledMemoryPiece(H264MemoryPiece * (&pList), H264MemoryPiece *pMem)


#ifndef DROP_MULTI_THREAD
H264Status H264SliceStore_::CopyData(MediaData * (&pSource))
{
    // this function copy incoming frame into internal buffer

    MediaDataEx *pMedia = &m_MediaDataEx;
    MediaDataEx::_MediaDataEx *pMediaEx = &m_MediaDataEx_;
    MediaDataEx *pMediaSrc = DynamicCast<MediaDataEx> (pSource);
    MediaDataEx::_MediaDataEx *pMediaSrcEx = pMediaSrc->GetExData();
    Ipp32s i;
    H264MemoryPiece *pMem;
    size_t nSourceSize, nAdvanceSize;
    bool bNonVCLUnit = false;

    if ((NAL_UT_SLICE <= (pMediaSrcEx->values[0] & NAL_UNITTYPE_BITS)) &&
        (NAL_UT_IDR_SLICE >= (pMediaSrcEx->values[0] & NAL_UNITTYPE_BITS)))
    {
        Ipp8u *pbSourcePointer = (Ipp8u *) pSource->GetDataPointer();
        H264Slice firstSlice, tempSlice;
        bool bIDRSlice = true;

        nSourceSize = pSource->GetDataSize();

        for (i = 0; i < (Ipp32s) pMediaSrcEx->count; i += 1)
        {
            if ((NAL_UT_SLICE <= (pMediaSrcEx->values[i] & NAL_UNITTYPE_BITS)) &&
                (NAL_UT_IDR_SLICE >= (pMediaSrcEx->values[i] & NAL_UNITTYPE_BITS)))
            {
                // slices must be from single picture
                if (0 == i)
                {
                    size_t nOffset;

                    firstSlice.m_pSeqParamSet = m_pSeqParamSet;
                    firstSlice.m_pPicParamSet = m_pPicParamSet;
                    nOffset = pMediaSrcEx->offsets[0] & -4;
                    firstSlice.ResetLimited(pbSourcePointer + nOffset,
                                            nSourceSize - nOffset);

                    if (INTRASLICE != firstSlice.m_SliceHeader.slice_type)
                        bIDRSlice = false;
                }
                else
                {
                    size_t nOffset;

                    tempSlice.m_pSeqParamSet = m_pSeqParamSet;
                    tempSlice.m_pPicParamSet = m_pPicParamSet;
                    nOffset = pMediaSrcEx->offsets[i] & -4;
                    tempSlice.ResetLimited(pbSourcePointer + nOffset,
                                           nSourceSize - nOffset);

                    if (INTRASLICE != tempSlice.m_SliceHeader.slice_type)
                        bIDRSlice = false;

                    if (false == IsPictureTheSame(&firstSlice, &tempSlice))
                        break;
                }

                pMediaEx->offsets[i] = pMediaSrcEx->offsets[i];
                pMediaEx->values[i] = pMediaSrcEx->values[i];
            }
            else
                break;
        }

        // checking for reference
        if (m_bWaitForIDR)
        {
            if (false == bIDRSlice)
                bNonVCLUnit = true;
            else
                m_bWaitForIDR = false;
        }
    }
    // the first unit is non-VCL unit
    else
    {
        pMediaEx->offsets[0] = pMediaSrcEx->offsets[0];
        pMediaEx->values[0] = pMediaSrcEx->values[0];
        bNonVCLUnit = true;
        i = 1;
    }

    // calculate source size
    if (i == (Ipp32s) pMediaSrcEx->count)
    {
        nSourceSize = ((pSource->GetDataSize() + 3) & -4);
        nAdvanceSize = pSource->GetDataSize();
    }
    else
    {
        nSourceSize = ((pMediaSrcEx->offsets[i] + 3) & -4) + 4;
        nAdvanceSize = (pMediaSrcEx->offsets[i] & -4);
    }

    pMem = GetFreeMemoryPiece(m_pFreeMemory, nSourceSize);
    if (NULL == pMem)
        return H264_ERROR;
    memcpy(pMem->m_pSourceBuffer, pSource->GetDataPointer(), nSourceSize);
    AddFilledMemoryPiece(m_pFilledMemory, pMem);

    pMediaEx->count = i;

    // update our media data
    pMedia->SetBufferPointer(pMem->m_pSourceBuffer, nSourceSize);
    pMedia->SetTime(pSource->GetTime());

    // update external media data
    {
        Ipp32s j = i;

        pSource->MoveDataPointer((Ipp32s) nAdvanceSize);

        for (; i < (Ipp32s) pMediaSrcEx->count; i += 1)
        {
            pMediaSrcEx->offsets[i - j] = (Ipp32s) (pMediaSrcEx->offsets[i] - nAdvanceSize);
            pMediaSrcEx->values[i - j] = pMediaSrcEx->values[i];
        }
        pMediaSrcEx->count = i - j;
    }

    // exchange media data
    pSource = pMedia;

    return (bNonVCLUnit) ? (H264_NON_VCL_NAL_UNIT) : (H264_OK);

} // H264Status H264SliceStore_::CopyData(MediaData * (&pSource))



H264Status H264SliceStore_::SeparateData(MediaData * (&pSource))
{
    MediaDataEx_2 *pMediaSrc = DynamicCast<MediaDataEx_2> (pSource);
    MediaDataEx_2::_MediaDataEx_2 *pMediaSrcEx = pMediaSrc->GetExData();
    bool bIDRSlice = true;
    bool bNonVCLUnit = false;
	H264Slice firstTmpSlice;

    firstTmpSlice.m_pSeqParamSet = m_pSeqParamSet;
    firstTmpSlice.m_pPicParamSet = m_pPicParamSet;
	firstTmpSlice.m_AdaptiveMarkingInfo_shadow = this->m_AdaptiveMarkingInfo_shadow;//***bnie: kinoma added 4/1/09
    
	
	firstTmpSlice.ResetLimited((unsigned char *)pMediaSrcEx->data_ptr_ary[0], pMediaSrcEx->size_ary[0]);

    if (INTRASLICE != firstTmpSlice.m_SliceHeader.slice_type)
        bIDRSlice = false;

    // checking for reference
    if (m_bWaitForIDR)
    {
        if (false == bIDRSlice)
            bNonVCLUnit = true;
        else
            m_bWaitForIDR = false;
    }

    return (bNonVCLUnit) ? (H264_NON_VCL_NAL_UNIT) : (H264_OK);

} // H264Status H264SliceStore_::SeparateData(MediaData * (&pSource))

H264Status  H264SliceStore_::PrepareNextSource(MediaData * (&pSource),
                                               H264VideoDecoder::H264DecoderFrame * (&pCurrentFrame),
                                               bool bEndOfStream)
{
    H264MemoryPiece *pMem = m_pFilledMemory;

#ifndef DROP_MULTI_THREAD
    // check parallelization status
    if ((false == bEndOfStream) &&
        (m_bDoFrameParallelization) &&
        (m_iAddedFrames - m_iProcessedFrames < 3))
        return H264_NOT_ENOUGH_DATA;
#endif

    // we require only first slice header to tell decoder
    // current parameters of video.

    // there is nothing to do
    if (0 == m_iSlicesInAdditionalQueue)
        return H264_NOT_ENOUGH_DATA;

    pSource = &m_MediaDataEx;

    pSource->SetBufferPointer(pMem->m_pSourceBuffer, pMem->m_nSourceSize);
    pCurrentFrame = m_pAdditionalQueue[0]->m_pCurrentFrame;

    return H264_OK;

} // h264Status H264SliceStore_::PrepareNextSource(MediaData * (&pSource),
#endif

//#define ONLY_SLICE_THREADING

#ifndef DROP_MULTI_THREAD  
bool H264SliceStore_::IsItTimeToFrameParallelization(void)
{
    // this function's target is to analyze decoding statistics
    // and to make decision "to do or not to do" frame-level parallelization

    /* DEBUG : need to implement */

#ifdef ONLY_SLICE_THREADING

        m_bDoFrameParallelization = false;

#else // !ONLY_SLICE_THREADING

    if (1 == m_iConsumerNumber)
        m_bDoFrameParallelization = false;
    else
        m_bDoFrameParallelization = true;

#endif // ONLY_SLICE_THREADING

    return m_bDoFrameParallelization;

} // bool H264SliceStore_::IsItTimeToFrameParallelization(void)
#endif

bool H264SliceStore_::AllocateMBIntraTypes(Ipp32s iIndex, Ipp32s iMBNumber)
{
    if ((NULL == m_pMBIntraTypes[iIndex]) ||
        (m_iMBIntraSizes[iIndex] < iMBNumber))
    {
        // delete previously allocated array
        if (m_pMBIntraTypes[iIndex])
            ippsFree_x(m_pMBIntraTypes[iIndex]);
        m_pMBIntraTypes[iIndex] = NULL;
        m_iMBIntraSizes[iIndex] = 0;

        m_pMBIntraTypes[iIndex] = (Ipp32u *) ippsMalloc_8u_x(iMBNumber * NUM_INTRA_TYPE_ELEMENTS * sizeof(*(m_pMBIntraTypes[iIndex])));
        if (NULL == m_pMBIntraTypes[iIndex])
            return false;
        m_iMBIntraSizes[iIndex] = iMBNumber;
    }

    return true;

} // bool H264SliceStore_::AllocateMBIntraTypes(Ipp32s iIndex, Ipp32s iMBNumber)

H264Status H264SliceStore_::AddSourceData(MediaData3_V51 * (&pSource) )//***bnie: , bool bOwnMemory)
{
    // in this function allowed only fullfilled MediaDataEx
    Ipp32s i, iNumber;
    Ipp32s iFirstSliceIndex = m_iSlicesInAdditionalQueue;
    //size_t nSliceSize;
    H264Status h264Res;

	//***bniebniebnie
	if( pSource->count == 0 )
		return H264_NOT_ENOUGH_DATA;

    // get starting slice number
    iNumber = GetFirstSliceNumber();

    // fill slices, slices info just for 1 frame
    for (i = 0; i < (Ipp32s) pSource->count; i += 1)
    {
        // allocate one more slice
        if ((NULL == m_pFreeSlices) && (false == AllocateSlice()))
            return H264_ERROR;

        // reset slice to initial state
        m_pFreeSlices->m_pPicParamSet = m_pPicParamSet;
        m_pFreeSlices->m_pSeqParamSet = m_pSeqParamSet;

		if (false == m_pFreeSlices->SetSourceData((unsigned char *)pSource->data_ptr_ary[i], pSource->bit_offset_ary[i], pSource->size_ary[i], iNumber))
            return H264_ERROR;

		if( i == 0 )
			m_pFreeSlices->m_AdaptiveMarkingInfo_shadow = this->m_AdaptiveMarkingInfo_shadow;//***bnie: kinoma added 4/1/09
		else if( !IsPictureTheSame(m_pAdditionalQueue[0], m_pFreeSlices))
			continue;	//***bnie: this slice is an alien 

		if (i == 0  && m_bWaitForIDR)
		{
			if (INTRASLICE != m_pFreeSlices->m_SliceHeader.slice_type)
				return H264_NON_VCL_NAL_UNIT;
			else
				m_bWaitForIDR = false;
		}

        // allocate decoding data
        if ((iFirstSliceIndex == m_iSlicesInAdditionalQueue) &&  (0 == (m_iAddedFrames & 1)))
        {
            Ipp32s iMBCount;
            H264Slice *pSlice = m_pFreeSlices;
            H264SeqParamSet *pSeqParam = pSlice->GetSeqParam();

            iMBCount = pSeqParam->frame_width_in_mbs * pSeqParam->frame_height_in_mbs;
            // allocate decoding data

#ifndef DROP_MULTI_THREAD
            if (m_bDoFrameParallelization)
                m_mbinfo[(m_iAddedFrames >> 1) & 1].Allocate(iMBCount);
            else
#endif
                m_mbinfo[0].Allocate(iMBCount);

            // allocate macroblock intra types
#ifndef DROP_MULTI_THREAD
			if (m_bDoFrameParallelization)
                AllocateMBIntraTypes((m_iAddedFrames >> 1) & 1, iMBCount);
            else
#endif
                AllocateMBIntraTypes(0, iMBCount);
        }

        // set local decoding info
#ifndef DROP_MULTI_THREAD
		if (m_bDoFrameParallelization)
			m_pFreeSlices->m_mbinfo = (m_mbinfo[(m_iAddedFrames >> 1) & 1]);
		else
#endif
			m_pFreeSlices->m_mbinfo = (m_mbinfo[0]);

#ifndef DROP_MULTI_THREAD
		if (m_bDoFrameParallelization)
			m_pFreeSlices->m_pMBIntraTypes = (m_pMBIntraTypes[(m_iAddedFrames >> 1) & 1]);
		else
#endif
			m_pFreeSlices->m_pMBIntraTypes = (m_pMBIntraTypes[0]);

#if 0
        // bind source location
        if ((bOwnMemory) && (m_iSlicesInAdditionalQueue == iFirstSliceIndex))
        {
            H264MemoryPiece *pMem = m_pFilledMemory;

            while (pMem->m_pNext)
                pMem = pMem->m_pNext;
            m_pFreeSlices->m_pSource = pMem;
        }
        else
#endif
            m_pFreeSlices->m_pSource = NULL;

        iNumber += 1;

        // correct maximum MB of previous slice
        if (m_iSlicesInAdditionalQueue  > iFirstSliceIndex)
        {
            if (false == m_pFreeSlices->IsSliceGroups())
                m_pAdditionalQueue[m_iSlicesInAdditionalQueue - 1]->m_iMaxMB = m_pFreeSlices->m_iFirstMB;
        }

        // add to list
        m_pAdditionalQueue[m_iSlicesInAdditionalQueue] = m_pFreeSlices;
        m_iSlicesInAdditionalQueue += 1;
        m_pFreeSlices = m_pFreeSlices->m_pNext;
    }

    // create return value
    h264Res = (m_iAddedFrames & 1) ? (H264_NEED_FREE_FIELD) : (H264_NEED_FREE_FRAME);

    // save last picture info
    m_iAddedFrames += 2;//***bnie: (0 != m_pAdditionalQueue[iFirstSliceIndex]->m_SliceHeader.field_pic_flag_kinoma_always_zero) ? (1) : (2);
    m_iSlicesInLastPic = m_iSlicesInAdditionalQueue - iFirstSliceIndex;

    return h264Res;

} // H264Status H264SliceStore_::AddSourceData(MediaData * (&pSource), bool bOwnMemory)

#if 0
H264Status H264SliceStore_::AddSource(MediaData * (&pSource) ) 
{
    AutomaticMutex guard(m_mGuard);

    if (NULL == pSource)
        return H264_OK;

    // single thread or we don't require frame level parallelization
#ifndef DROP_MULTI_THREAD  
	if (false == IsItTimeToFrameParallelization())
#endif
	{
        H264Status h264Res;

            h264Res = AddSourceData(pSource);

        return h264Res;
    }

#ifndef DROP_MULTI_THREAD
    // do frame parallelization
	{
        H264Status h264Res;

        // separate data into part & copy it to internal buffer
        //***bnie if (false == bDataSwapped)
        //***bnie     h264Res = SwapData(pSource);
        //***bnie else
            h264Res = CopyData(pSource);

        // add slices to queue
        if (H264_OK == h264Res)
            h264Res = AddSourceData(pSource, true);

        if (H264_ERROR == h264Res)
            return H264_ERROR;

        // to avoid ocassionaly changing
        pSource = NULL;

        return h264Res;
    }
#endif

} // H264Status H264SliceStore_::AddSource(MediaData * (&pSource), bool bDataSwapped)
#endif


H264Status H264SliceStore_::AddFreeFrame(H264VideoDecoder::H264DecoderFrame *pFrame)
{
    Ipp32s i;

    for (i = m_iSlicesInAdditionalQueue - 1; i >= 0; i -= 1)
    {
        H264Slice *pSlice = m_pAdditionalQueue[i];

        if (NULL == pSlice->m_pCurrentFrame)
        {
           //*** pFrame->m_dFrameTime = pSlice->m_dTime;
            pSlice->m_pCurrentFrame = pFrame;
        }
        else
            break;
    }

    //
    // do frame preparation
    //

    {
        Status umcRes;
        H264Slice *pSlice = m_pAdditionalQueue[m_iSlicesInAdditionalQueue - 1];
        H264SeqParamSet *pSeqParam = pSlice->GetSeqParam();
        sDimensions dimensions;
        Ipp32s iMBWidth = pSeqParam->frame_width_in_mbs;
        Ipp32s iMBHeight = pSeqParam->frame_height_in_mbs;

        dimensions.width = iMBWidth * 16;
        dimensions.height = iMBHeight * 16;

        // Allocate the frame data
        umcRes = pFrame->allocate(dimensions,
                                  MAX(pSeqParam->bit_depth_luma, pSeqParam->bit_depth_chroma),
                                  pSeqParam->chroma_format_idc);
        if (umcRes != UMC_OK)
            return H264_ERROR;

        pFrame->m_crop_left = pSeqParam->frame_cropping_rect_left_offset;
        pFrame->m_crop_right =pSeqParam->frame_cropping_rect_right_offset;
        pFrame->m_crop_top = pSeqParam->frame_cropping_rect_top_offset;//bnie:  * 1(2 - pSeqParam->frame_mbs_only_flag_kinoma_always_one);
        pFrame->m_crop_bottom = pSeqParam->frame_cropping_rect_bottom_offset;//bnie: * (2 - pSeqParam->frame_mbs_only_flag_kinoma_always_one);
        pFrame->m_crop_flag = pSeqParam->frame_cropping_flag;
       
        {
            {
               //***bnie:  pFrame->m_PictureStructureForRef =  pFrame->m_PictureStructureForDec = FRM_STRUCTURE;
            }
        }

        pFrame->totalMBs = iMBWidth * iMBHeight;

        // reset frame global data
        {
            Ipp32s x, y;
            H264DecoderMacroblockGlobalInfo *pMBInfo = pFrame->m_mbinfo.mbs;

            for (y = 0; y < iMBHeight; y += 1)
            {
                //***bnie: Ipp8u bVal = (Ipp8u) ((pSlice->m_SliceHeader.MbaffFrameFlag & y) << 1);

                for (x = 0; x < iMBWidth; x += 1)
                {
                    /* DEBUG : we can avoid set slice_id variable for non-slice-group case */
                    pMBInfo[x].slice_id = -1;

                    pMBInfo[x].mb_aux_fields = 0;//***bniebnie: bVal;
                }
                pMBInfo += x;
            }
        }
    }

    return H264_OK;

} // H264Status H264SliceStore_::AddFreeFrame(H264VideoDecoder::H264DecoderFrame *pFrame)

bool H264SliceStore_::AllocateSlice(void)
{
    // this is guarded function, safe to touch any variable
    H264Slice *pTemp;

    pTemp = new H264Slice();
    if (NULL == pTemp)
        return false;

    // initialize slice
    if (false == pTemp->Init(m_iConsumerNumber))
    {
        delete pTemp;
        return false;
    }

    // bound to list
    pTemp->m_pNext = m_pFreeSlices;
    m_pFreeSlices = pTemp;

    return true;

} // bool H264SliceStore_::AllocateSlice(void)


Ipp32s H264SliceStore_::GetFirstSliceNumber(void)
{
    if (0 == (m_iAddedFrames & 1))
        return 0;
    else
        return m_iSlicesInLastPic;

} // Ipp32s H264SliceStore_::GetFirstSliceNumber(void)

void H264SliceStore_::SetDestinationFrame(H264VideoDecoder::H264DecoderFrame *, H264DecoderLocalMacroblockDescriptor &)
{/*
    AutomaticMutex guard(m_mGuard);
    Ipp32s i;
    Ipp32s iFirstSliceFrameNum = 0;

    for (i = 0; i < m_iSlicesInAdditionalQueue; i += 1)
    {
        H264Slice *pSlice = m_pAdditionalQueue[i];
        Ipp32s iFrameNum;

        iFrameNum = GetFrameNum(pSlice);
        iFirstSliceFrameNum = (0 == i) ? (iFrameNum) : (iFirstSliceFrameNum);

        if (iFirstSliceFrameNum == iFrameNum)
            pSlice->m_mbinfo = mbinfo;
        else
            break;
    }
*/
    //Ipp8u *p = NULL;
    //*p = 0;

} // void H264SliceStore_::SetDestinationFrame(H264VideoDecoder::H264DecoderFrame *pCurrentFrame, H264DecoderLocalMacroblockDescriptor &mbinfo)

void H264SliceStore_::SwitchFrame(void)
{
#ifndef DROP_MULTI_THREAD
    AutomaticMutex guard(m_mGuard);
#endif
	Ipp32s i;

    // copy slices to main queue
    m_iSlicesInMainQueue = 0;
    for (i = 0; i < m_iSlicesInAdditionalQueue; i += 1)
    {
        H264Slice *pSlice = m_pAdditionalQueue[i];

        if ((0 == i) ||
            (IsPictureTheSame(m_pMainQueue[0], pSlice)))
        {
            // set flag to update references
            pSlice->m_bNeedToUpdateRefPicList = true;
            m_pMainQueue[i] = pSlice;
            m_pAdditionalQueue[i] = NULL;
            m_iSlicesInMainQueue += 1;
        }
        else
            break;
    }

    // move slices up in additional queue
    {
        Ipp32s iWriteIndex = 0;
        Ipp32s iAdditionalSlices = m_iSlicesInAdditionalQueue - i;

        for (; i < m_iSlicesInAdditionalQueue; i += 1)
        {
            m_pAdditionalQueue[iWriteIndex] = m_pAdditionalQueue[i];
            m_pAdditionalQueue[i] = NULL;
            iWriteIndex += 1;
        }

        m_iSlicesInAdditionalQueue = iAdditionalSlices;
    }

    m_bFirstDebThreadedCall = true;
    m_bDoFrameDeblocking = true;

} // void H264SliceStore_::SwitchFrame(void)

void H264SliceStore_::RecycleSlice(void)
{
#ifndef DROP_MULTI_THREAD
    AutomaticMutex guard(m_mGuard);
#endif
	Ipp32s i;
    //***bnie: size_t nSize;
    H264Slice *pSlice = NULL;

    // find last slice
    if (m_iSlicesInMainQueue)
        pSlice = m_pMainQueue[m_iSlicesInMainQueue - 1];
    else
        pSlice = NULL;

#if 0 //***bnie:
    if (pSlice)
    {
        H264Bitstream *pBitStream = pSlice->GetBitStream();
        Ipp32u *pSrc, nOffset;

        pBitStream->GetState(&pSrc, &nOffset);

        nSize = ((Ipp8u *) pSrc) - m_pbSource;
    }
    else
        nSize = 0;
#endif

    // move used slice(s) to free queue
    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        pSlice = m_pMainQueue[i];

        // move source memory to free queue
        if (pSlice->m_pSource)
        {
            H264MemoryPiece *pMem;

            VM_ASSERT(pSlice->m_pSource == m_pFilledMemory);

            pMem = m_pFilledMemory;
            m_pFilledMemory = m_pFilledMemory->m_pNext;
            pMem->m_pNext = m_pFreeMemory;
            m_pFreeMemory = pMem;
        }

        m_pMainQueue[i] = NULL;
        pSlice->m_pNext = m_pFreeSlices;
        m_pFreeSlices = pSlice;
    }
    m_iSlicesInMainQueue = 0;

    // increment number of decoded frames
    if (pSlice)
        m_iProcessedFrames += 1;

    //***bnie: return nSize;

} // size_t H264SliceStore_::GetUsedSize(void)

bool H264SliceStore_::GetNextSlice(H264Task *pTask)
{
#ifndef DROP_MULTI_THREAD
    AutomaticMutex guard(m_mGuard);
#endif

    // check error(s)
    if ((NULL == pTask) || (0 >= m_iSlicesInMainQueue))
        return false;

    // try to get slice to decode
    if ((false == m_pMainQueue[0]->IsSliceGroups()) || (0 == pTask->m_iThreadNumber))
    {
        if (GetNextSliceToDecoding(pTask))
            return true;
    }

    // try to get slice to deblock
    if ((false == m_pMainQueue[0]->IsSliceGroups()) || (0 == pTask->m_iThreadNumber))
        return GetNextSliceToDeblocking(pTask);

    return false;

} // bool H264SliceStore_::GetNextSlice(H264Task *pTask)

bool H264SliceStore_::GetNextSliceToDecoding(H264Task *pTask)
{
    // this is guarded function, safe to touch any variable

    Ipp32s i;
    bool bDoDeblocking;

    // skip some slices, more suitable for first thread
    // and first slice is always reserved for first slice decoder
    if (pTask->m_iThreadNumber)
    {
        i = max(1, m_iSlicesInMainQueue / m_iConsumerNumber);
        bDoDeblocking = false;
    }
    else
    {
        i = 0;
        bDoDeblocking = true;
    }

    // find first uncompressed slice
    for (; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if ((false == pSlice->m_bInProcess) && (false == pSlice->m_bDecoded))
        {
            pTask->m_bDone = false;
            pTask->m_iFirstMB = pSlice->m_iFirstMB;
            pTask->m_iMaxMB = pSlice->m_iMaxMB;
            pTask->m_iMBToProcess = min(pSlice->m_iMaxMB - pSlice->m_iFirstMB, pSlice->m_iAvailableMB);
            pTask->m_iTaskID = TASK_PROCESS;
            pTask->m_pBuffer = NULL;
            pTask->m_pBufferEnd = NULL;
            pTask->m_pSlice = pSlice;
            pTask->pFunctionSingle = &H264SegmentDecoder::ProcessSlice;
#ifndef DROP_MULTI_THREAD          
			pTask->pFunctionMulti = NULL;
#endif
            // we can do deblocking only on independent slices or
            // when all previous slices are deblocked
            if (DEBLOCK_FILTER_ON != pSlice->m_SliceHeader.disable_deblocking_filter_idc)
                bDoDeblocking = true;
            pSlice->m_bPrevDeblocked = bDoDeblocking;
            pSlice->m_bInProcess = true;
            pSlice->m_bDecVacant = false;
            pSlice->m_bRecVacant = false;
            pSlice->m_bDebVacant = false;

            // we need to update reference list in thread safe mode
            pSlice->UpdateReferenceList(m_pDecoderFrameList);

            return true;
        }
    }

    return false;

} // bool H264SliceStore_::GetNextSliceToDecoding(H264Task *pTask)

bool H264SliceStore_::GetNextSliceToDeblocking(H264Task *pTask)
{
    // this is guarded function, safe to touch any variable

    bool bSliceGroups = m_pMainQueue[0]->IsSliceGroups();

    // slice group deblocking
    if (bSliceGroups)
    {
        Ipp32s iFirstMB = m_pMainQueue[0]->m_iFirstMB;
        bool bNothingToDo = true;
        Ipp32s i;

        for (i = 0; i < m_iSlicesInMainQueue; i += 1)
        {
            H264Slice *pSlice = m_pMainQueue[i];

            VM_ASSERT(false == pSlice->m_bInProcess);

            pSlice->m_bInProcess = true;
            pSlice->m_bDebVacant = false;
            iFirstMB = min(iFirstMB, pSlice->m_iFirstMB);
            if (false == pSlice->m_bDeblocked)
                bNothingToDo = false;
        }

        // we already deblocked
        if (bNothingToDo)
            return false;

        pTask->m_bDone = false;
        pTask->m_iFirstMB = iFirstMB;
        {
            H264Slice *pSlice = m_pMainQueue[0];
            Ipp32s iMBInFrame = ( pSlice->m_iMBWidth * pSlice->m_iMBHeight);
								//***bnie:	/ ((pSlice->m_SliceHeader.field_pic_flag) ? (2) : (1) );
            pTask->m_iMaxMB = iFirstMB + iMBInFrame;
            pTask->m_iMBToProcess = iMBInFrame;
        }
        pTask->m_iTaskID = TASK_DEB_FRAME;
        pTask->m_pBuffer = NULL;
        pTask->m_pBufferEnd = NULL;
        pTask->m_pSlice = m_pMainQueue[0];
#ifndef DROP_MULTI_THREAD
        pTask->pFunctionMulti = NULL;
#endif
        pTask->pFunctionSingle = &H264SegmentDecoder::DeblockSlice;

#ifdef ECHO_DEB
        {
            char cStr[256];
            sprintf(cStr, "(%d) frame deb - % 4d to % 4d\n",
                pTask->m_iThreadNumber,
                pTask->m_iFirstMB,
                pTask->m_iFirstMB + pTask->m_iMBToProcess);
            OutputDebugString(cStr);
        }
#endif // ECHO_DEB

        return true;
    }
    else
    {
        Ipp32s i;
        bool bPrevDeblocked = true;

        for (i = 0; i < m_iSlicesInMainQueue; i += 1)
        {
            H264Slice *pSlice = m_pMainQueue[i];

            // we can do deblocking only on vacant slices
            if ((false == pSlice->m_bInProcess) &&
                (true == pSlice->m_bDecoded) &&
                (false == pSlice->m_bDeblocked))
            {
                // we can do this only when previous slice was deblocked or
                // deblocking isn't going through slice boundaries
                if ((true == bPrevDeblocked) ||
                    (false == pSlice->DeblockThroughBoundaries()))
                {
                    pTask->m_bDone = false;
                    pTask->m_iFirstMB = pSlice->m_iFirstMB;
                    pTask->m_iMaxMB = pSlice->m_iMaxMB;
                    pTask->m_iMBToProcess = pSlice->m_iMaxMB - pSlice->m_iFirstMB;
                    pTask->m_iTaskID = TASK_DEB_SLICE;
                    pTask->m_pBuffer = NULL;
                    pTask->m_pBufferEnd = NULL;
                    pTask->m_pSlice = pSlice;
#ifndef DROP_MULTI_THREAD
                    pTask->pFunctionMulti = NULL;
#endif
					pTask->pFunctionSingle = &H264SegmentDecoder::DeblockSlice;

                    pSlice->m_bPrevDeblocked = true;
                    pSlice->m_bInProcess = true;
                    pSlice->m_bDebVacant = false;

#ifdef ECHO_DEB
                    {
                        char cStr[256];
                        sprintf(cStr, "(%d) slice deb - % 4d to % 4d\n",
                            pTask->m_iThreadNumber,
                            pTask->m_iFirstMB,
                            pTask->m_iFirstMB + pTask->m_iMBToProcess);
                        OutputDebugString(cStr);
                    }
#endif // ECHO_DEB

                    return true;
                }
            }

            // save previous slices deblocking condition
            if (false == pSlice->m_bDeblocked)
                bPrevDeblocked = false;
        }
    }

    return false;

} // bool H264SliceStore_::GetNextSliceToDeblocking(H264Task *pTask)

Ipp32s H264SliceStore_::CountDecodingBuffers(void)
{
    Ipp32s i;
    Ipp32s iCount = 0;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];
        Ipp32s j;

        for (j = 0; j < NUMBER_OF_ROWS; j += 1)
        {
            if (pSlice->m_pDecBuffer[j])
                iCount += 1;
            else
                break;
        }
    }

    return iCount;

} // Ipp32s H264SliceStore_::CountDecodingBuffers(void)

Ipp32s H264SliceStore_::CountReconstructBuffers(void)
{
    Ipp32s i;
    Ipp32s iCount = 0;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];
        Ipp32s j;

        if (pSlice->m_pAddBuffer)
        {
            iCount = 0;
            break;
        }

        for (j = 0; j < NUMBER_OF_ROWS; j += 1)
        {
            if (pSlice->m_pRecBuffer[j])
                iCount += 1;
            else
                break;
        }
    }

    return iCount;

} // Ipp32s H264SliceStore_::CountReconstructBuffers(void)

bool H264SliceStore_::AreManyToDeblock(void)
{
    Ipp32s iToDeb = 0, iToRec = 0;
    Ipp32s i;

    // find current macroblock to deblock
    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        iToDeb = pSlice->m_iCurMBToDeb;
        if (false == pSlice->m_bDeblocked)
            break;
    }

    // find current macroblock to reconstruct
    for (; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        iToRec = pSlice->m_iCurMBToRec;
        if (false == pSlice->m_bDecoded)
            break;
    }

    if ((iToRec - iToDeb) >= m_pMainQueue[0]->GetMBRowWidth() * 3)
        return true;

    return false;

} // bool H264SliceStore_::AreManyToDeblock(void)

Ipp32s H264SliceStore_::GetNumberOfSlicesToDecode(void)
{
    Ipp32s i, iCount = 0;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if (pSlice->m_iMaxMB > pSlice->m_iCurMBToDec)
            iCount += 1;
    }

    return iCount;

} // Ipp32s H264SliceStore_::GetNumberOfSlicesToDecode(void)

Ipp32s H264SliceStore_::GetNumberOfSlicesToReconstruct(void)
{
    Ipp32s i, iCount = 0;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if (pSlice->m_iMaxMB > pSlice->m_iCurMBToRec)
            iCount += 1;
    }

    return iCount;

} // Ipp32s H264SliceStore_::GetNumberOfSlicesToReconstruct(void)

#ifndef DROP_MULTI_THREAD

bool H264SliceStore_::GetNextTask(H264Task *pTask)
{
    AutomaticMutex guard(m_mGuard);
#ifdef TIME
    CStarter start(pTask->m_iThreadNumber);
#endif // TIME
    bool bOk = true;

    // check error(s)
    if ((NULL == pTask) ||
        (0 >= m_iSlicesInMainQueue))
        return false;

    if (m_pMainQueue[0]->IsSliceGroups())
    {
        // avoid double lock mutex
        guard.Unlock();

        return GetNextSlice(pTask);
    }

    while (true == bOk)
    {
        Ipp32s iSlicesToDecode;
        Ipp32s iSliceToReconstruct;
        bool bDecompessed;

        // get current decoding status
        iSlicesToDecode = GetNumberOfSlicesToDecode();
        iSliceToReconstruct = GetNumberOfSlicesToReconstruct();
        bDecompessed = (0 == iSliceToReconstruct);

        // check frame state
        if ((bDecompessed) &&
            (IsFrameDeblocked()))
            return false;

        // sometimes we have decoders more then available work.
        // let exit all extra decoders.
        if (NUMBER_OF_DEBLOCKERS <= pTask->m_iThreadNumber)
        {
            // all other threads must exit after their job will be done
            if (pTask->m_iThreadNumber & 1)
            {
                if (iSlicesToDecode <= pTask->m_iThreadNumber / 2)
                    return false;
            }
            else
            {
                if (iSliceToReconstruct <= pTask->m_iThreadNumber / 2)
                {
                    // keep one extra thread to do deblocking.
                    // when we have many deblocking threads we don't require in that thread.
                    if (min(NUMBER_OF_DEBLOCKERS, 2) == pTask->m_iThreadNumber)
                    {
                        if (0 == iSlicesToDecode)
                            return false;
                    }
                    else
                        return false;
                }
            }
        }

        // there is time to do threaded deblocking
        if (bDecompessed)
        {
            if (NUMBER_OF_DEBLOCKERS <= pTask->m_iThreadNumber)
                return false;

            if (m_bDoFrameDeblocking)
            {
                if (true == GetFrameDeblockingTaskThreaded(pTask))
                    return true;
            }
            else
            {
                if (true == GetSliceDeblockingTaskThreaded(pTask))
                    return true;
            }
        }
        // just get next task
        else
        {
            // odd thread
            if (pTask->m_iThreadNumber & 1)
            {/*
                // try to get decoding task from additional queue
                if (true == GetAdditionalDecodingTask(pTask))
                    return true;*/

                if (CountReconstructBuffers() > CountDecodingBuffers())
                {
                    // try to get deblocking task from main queue
                    if (true == GetDeblockingTask(pTask))
                        return true;
                }

                // try to get decoding task from main frame
                if (true == GetDecodingTask(pTask, CURRENT_SLICE))
                    return true;

                // try to get deblocking task from main queue
                if (true == GetDeblockingTask(pTask))
                    return true;

                // try to get reconstruct task from main queue
                if (true == GetReconstructTask(pTask))
                    return true;

                // try to get decoding task from main queue
                if (true == GetDecodingTask(pTask, OTHER_SLICES))
                    return true;
            }
            // even thread
            else
            {
                if ((CountReconstructBuffers() <= CountDecodingBuffers()) ||
                    (AreManyToDeblock()))
                {
                    // try to get deblocking task from main queue
                    if (true == GetDeblockingTask(pTask))
                        return true;
                }

                // try to get reconstruct task from main queue
                if (true == GetReconstructTask(pTask))
                    return true;

                // try to get deblocking task from main queue
                if (true == GetDeblockingTask(pTask))
                    return true;

                // try to get decoding task from main queue
                if (true == GetDecodingTask(pTask, OTHER_SLICES))
                    return true;

                // try to get decoding task from main queue
                if (true == GetDecodingTask(pTask, CURRENT_SLICE))
                    return true;
            }
        }

        // All threads allowed to touch additional queue.
        // Of course we must be so far of finishing current frame.
        if (iSliceToReconstruct)
        {
            // try to get decoding task from additional queue
            if (true == GetAdditionalDecodingTask(pTask))
                return true;
        }

#ifdef TIME
        timer.SleepStart(pTask->m_iThreadNumber);
#endif // TIME

        // it is time to sleep
        m_nWaitingThreads |= (1 << (pTask->m_iThreadNumber));
        guard.Unlock();

        m_eWaiting[pTask->m_iThreadNumber]->Wait();

        guard.Lock();
        m_nWaitingThreads ^= (1 << (pTask->m_iThreadNumber));

#ifdef TIME
        timer.SleepStop(pTask->m_iThreadNumber);
#endif // TIME
    }

    return false;

} // bool H264SliceStore_::GetNextTask(H264Task *pTask)

#endif

H264SliceHeader *H264SliceStore_::GetSliceHeader(Ipp32s iSliceNumber)
{
    // this isn't guarded function, safe to touch only slice headers

    Ipp32s i;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice;

        pSlice = m_pMainQueue[i];
        if ((pSlice) &&
            (iSliceNumber == pSlice->GetSliceNum()))
            return pSlice->GetSliceHeader();
    }

    return NULL;

} // H264SliceHeader *H264SliceStore_::GetSliceHeader(Ipp32s iSliceNumber)

bool H264SliceStore_::GetDecodingTask(H264Task *pTask, Ipp32s iMode)
{
    // this is guarded function, safe to touch any variable

    Ipp32s i;

    // find current slice
    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if (pSlice->m_iMaxMB > pSlice->m_iCurMBToDec)
            break;
    }
    if (i == m_iSlicesInMainQueue)
        return false;

    if (CURRENT_SLICE == iMode)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if ((false == pSlice->m_bDecoded) &&
            (true == pSlice->m_bDecVacant) &&
            (pSlice->m_pDecBuffer[0]))
        {
            Ipp32s iMBWidth = pSlice->GetMBRowWidth();

            pTask->m_bDone = false;
            pTask->m_iFirstMB = pSlice->m_iCurMBToDec;
            pTask->m_iMaxMB = pSlice->m_iMaxMB;
            pTask->m_iMBToProcess = min(pSlice->m_iCurMBToDec -
                                        (pSlice->m_iCurMBToDec % iMBWidth) +
                                        iMBWidth,
                                        pSlice->m_iMaxMB) - pSlice->m_iCurMBToDec;
            pTask->m_iMBToProcess = min(pTask->m_iMBToProcess, pSlice->m_iAvailableMB);
            pTask->m_iTaskID = TASK_DEC;
            pTask->m_pBuffer = pSlice->m_pDecBuffer[0];
            pTask->m_pBufferEnd = NULL;
            pTask->m_pSlice = pSlice;
#ifndef DROP_MULTI_THREAD
            pTask->pFunctionMulti = &H264SegmentDecoderMultiThreaded::DecodeSegment;
#endif
			pTask->pFunctionSingle = NULL;

            pSlice->m_bInProcess = true;
            pSlice->m_bDecVacant = false;

            /* DEBUG : CAVLC isn't frame parallelized */
            if (0 == pSlice->GetPicParam()->entropy_coding_mode)
                pSlice->UpdateReferenceList(m_pDecoderFrameList);

            return true;
        }

        return false;
    }

    // find any decoding task
    for (i += 1; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if ((false == pSlice->m_bDecoded) &&
            (true == pSlice->m_bDecVacant) &&
            (pSlice->m_iCurMBToDec < pSlice->m_iMaxMB) &&
            (pSlice->m_pDecBuffer[0]))
        {
            Ipp32s iMBWidth = pSlice->GetMBRowWidth();

            pTask->m_bDone = false;
            pTask->m_iFirstMB = pSlice->m_iCurMBToDec;
            pTask->m_iMaxMB = pSlice->m_iMaxMB;
            pTask->m_iMBToProcess = min(pSlice->m_iCurMBToDec -
                                        (pSlice->m_iCurMBToDec % iMBWidth) +
                                        iMBWidth,
                                        pSlice->m_iMaxMB) - pSlice->m_iCurMBToDec;
            pTask->m_iMBToProcess = min(pTask->m_iMBToProcess, pSlice->m_iAvailableMB);
            pTask->m_iTaskID = TASK_DEC;
            pTask->m_pBuffer = pSlice->m_pDecBuffer[0];
            pTask->m_pBufferEnd = NULL;
            pTask->m_pSlice = pSlice;
#ifndef DROP_MULTI_THREAD
            pTask->pFunctionMulti = &H264SegmentDecoderMultiThreaded::DecodeSegment;
#endif
			pTask->pFunctionSingle = NULL;

            pSlice->m_bInProcess = true;
            pSlice->m_bDecVacant = false;

            /* DEBUG : CAVLC isn't frame parallelized */
            if (0 == pSlice->GetPicParam()->entropy_coding_mode)
                pSlice->UpdateReferenceList(m_pDecoderFrameList);

            return true;
        }
    }

    return false;

} // bool H264SliceStore_::GetDecodingTask(H264Task *pTask, Ipp32s iMode)

bool H264SliceStore_::GetReconstructTask(H264Task *pTask)
{
    // this is guarded function, safe to touch any variable

    Ipp32s i;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if ((false == pSlice->m_bDecoded) &&
            (true == pSlice->m_bRecVacant) &&
            (pSlice->m_pRecBuffer[0] || pSlice->m_pAddBuffer))
        {
            Ipp32s iMBWidth = pSlice->GetMBRowWidth();

            pTask->m_bDone = false;
            pTask->m_iFirstMB = pSlice->m_iCurMBToRec;
            pTask->m_iMaxMB = pSlice->m_iMaxMB;
            pTask->m_iMBToProcess = min(pSlice->m_iCurMBToRec -
                                        (pSlice->m_iCurMBToRec % iMBWidth) +
                                        iMBWidth,
                                        pSlice->m_iMaxMB) - pSlice->m_iCurMBToRec;
            pTask->m_iTaskID = TASK_REC;
            pTask->m_pBuffer = (pSlice->m_pAddBuffer) ? ((Ipp16s *) pSlice->m_pAddBuffer->GetPointer()) : (pSlice->m_pRecBuffer[0]);
            pTask->m_pBufferEnd = NULL;
            pTask->m_pSlice = pSlice;
#ifndef DROP_MULTI_THREAD
            pTask->pFunctionMulti = &H264SegmentDecoderMultiThreaded::ReconstructSegment;
#endif
			pTask->pFunctionSingle = NULL;

            pSlice->m_bRecVacant = false;

            // we need to update reference list in thread safe mode
            pSlice->UpdateReferenceList(m_pDecoderFrameList);

#ifdef ECHO
            {
                char cStr[256];
                sprintf(cStr, "(%d) rec - % 4d to % 4d\n",
                    pTask->m_iThreadNumber,
                    pTask->m_iFirstMB,
                    pTask->m_iFirstMB + pTask->m_iMBToProcess);
                OutputDebugString(cStr);
            }
#endif // ECHO

            return true;
        }
    }

    return false;

} // bool H264SliceStore_::GetReconstructTask(H264Task *pTask)

bool H264SliceStore_::GetDeblockingTask(H264Task *pTask)
{
    // this is guarded function, safe to touch any variable

    Ipp32s i;
    bool bPrevDeblocked = true;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if (pSlice)
        {
            Ipp32s iMBWidth = pSlice->GetMBRowWidth();

            if ((false == pSlice->m_bDeblocked) &&
                ((true == bPrevDeblocked) || (false == pSlice->DeblockThroughBoundaries())) &&
                (true == pSlice->m_bDebVacant) &&
                ((pSlice->m_iCurMBToRec - pSlice->m_iCurMBToDeb > iMBWidth) || (pSlice->m_iCurMBToRec == pSlice->m_iMaxMB)))
            {
                Ipp32s iDebUnit = 1;//***bniebnie: (pSlice->GetSliceHeader()->MbaffFrameFlag) ? (2) : (1);

                pTask->m_bDone = false;
                pTask->m_iFirstMB = pSlice->m_iCurMBToDeb;
                pTask->m_iMaxMB = pSlice->m_iMaxMB;
                {
                    pTask->m_iMBToProcess = min(pSlice->m_iCurMBToDeb -
                                                (pSlice->m_iCurMBToDeb % iMBWidth) +
                                                iMBWidth,
                                                pSlice->m_iMaxMB) - pSlice->m_iCurMBToDeb;
                    pTask->m_iMBToProcess = min(pTask->m_iMBToProcess,
                                                iMBWidth / NUMBER_OF_PIECES);
                    pTask->m_iMBToProcess = max(iDebUnit, pTask->m_iMBToProcess);
                    pTask->m_iMBToProcess = align_value<Ipp32s> (pTask->m_iMBToProcess, iDebUnit);
                }
                pTask->m_iTaskID = TASK_DEB;
                pTask->m_pBuffer = NULL;
                pTask->m_pBufferEnd = NULL;
                pTask->m_pSlice = pSlice;
#ifndef DROP_MULTI_THREAD
				pTask->pFunctionMulti = &H264SegmentDecoderMultiThreaded::DeblockSegment;
#endif
				pTask->pFunctionSingle = NULL;

                pSlice->m_bDebVacant = false;

#ifdef ECHO_DEB
            {
                char cStr[256];
                sprintf(cStr, "(%d) deb - % 4d to % 4d\n",
                    pTask->m_iThreadNumber,
                    pTask->m_iFirstMB,
                    pTask->m_iFirstMB + pTask->m_iMBToProcess);
                OutputDebugString(cStr);
            }
#endif // ECHO_DEB

                return true;
            }

            // save previous slices deblocking condition
            if (false == pSlice->m_bDeblocked)
                bPrevDeblocked = false;
        }
    }

    return false;

} // bool H264SliceStore_::GetDeblockingTask(H264Task *pTask)

bool H264SliceStore_::GetFrameDeblockingTaskThreaded(H264Task *pTask)
{
    // this is guarded function, safe to touch any variable

    if (m_bFirstDebThreadedCall)
    {
        Ipp32s i;
        Ipp32s iFirstMB = -1, iMaxMB = -1;
        Ipp32s iMBWidth = m_pMainQueue[0]->GetMBRowWidth();
        Ipp32s iDebUnit = 1;//***bniebnie: (m_pMainQueue[0]->m_SliceHeader.MbaffFrameFlag) ? (2) : (1);

        // check all other threads are sleep
        for (i = 0; i < m_iSlicesInMainQueue; i += 1)
        {
            H264Slice *pSlice = m_pMainQueue[i];

            if ((pSlice) &&
                (false == pSlice->m_bDebVacant))
                return false;
        }

        // handle little slices
        if (iDebUnit * 2 > iMBWidth / m_iConsumerNumber)
            return GetDeblockingTask(pTask);

        // calculate deblocking range
        for (i = 0; i < m_iSlicesInMainQueue; i += 1)
        {
            H264Slice *pSlice = m_pMainQueue[i];

            if ((pSlice) &&
                (false == pSlice->m_bDeblocked))
            {
                if (-1 == iFirstMB)
                    iFirstMB = pSlice->m_iCurMBToDeb;
                if (iMaxMB < pSlice->m_iMaxMB)
                    iMaxMB = pSlice->m_iMaxMB;
            }
        }

        m_DebTools.Reset(iFirstMB,
                         iMaxMB,
                         iDebUnit,
                         iMBWidth);

        m_bFirstDebThreadedCall = false;
    }

    // get next piece to deblock
    if (false == m_DebTools.GetMBToProcess(pTask))
        return false;

    // correct task to slice range
    {
        Ipp32s i;

        for (i = 0; i < m_iSlicesInMainQueue; i += 1)
        {
            H264Slice *pSlice = m_pMainQueue[i];

            if ((pTask->m_iFirstMB >= pSlice->m_iFirstMB) &&
                (pTask->m_iFirstMB < pSlice->m_iMaxMB))
            {
                pTask->m_iTaskID = TASK_DEB_FRAME_THREADED;
                pTask->m_pSlice = pSlice;
                if (pTask->m_iFirstMB + pTask->m_iMBToProcess > pSlice->m_iMaxMB)
                    pTask->m_iMBToProcess = pSlice->m_iMaxMB - pTask->m_iFirstMB;
                break;
            }
        }
    }

    return true;

} // bool H264SliceStore_::GetFrameDeblockingTaskThreaded(H264Task *pTask)

#ifndef DROP_MULTI_THREAD
bool H264SliceStore_::GetSliceDeblockingTaskThreaded(H264Task *pTask)
{
    // this is guarded function, safe to touch any variable

    Ipp32s i;
    bool bPrevDeblocked = true;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if (pSlice)
        {
            if ((false == pSlice->m_bDeblocked) &&
                ((true == bPrevDeblocked) || (false == pSlice->DeblockThroughBoundaries())) &&
                (pSlice->m_bDebVacant))
            {
                Ipp32s iMBWidth = pSlice->GetMBRowWidth();
                Ipp32s iDebUnit = 1;//***bniebnie: (pSlice->GetSliceHeader()->MbaffFrameFlag) ? (2) : (1);

                // handle little slices
                if (2 * iDebUnit > iMBWidth / m_iConsumerNumber)
                    return GetDeblockingTask(pTask);

                // reset threaded deblocking tools
                if (pSlice->m_bFirstDebThreadedCall)
                {
                    pSlice->m_DebTools.Reset(pSlice->m_iCurMBToDeb, pSlice->m_iMaxMB, iDebUnit, iMBWidth);
                    pSlice->m_bFirstDebThreadedCall = false;
                }

                // get next piece to deblock
                if (true == pSlice->m_DebTools.GetMBToProcess(pTask))
                {
                    pTask->m_pSlice = pSlice;
                    return true;
                }
            }

            // save previous slices deblocking condition
            if (false == pSlice->m_bDeblocked)
                bPrevDeblocked = false;
        }
    }

    return false;

} // bool H264SliceStore_::GetSliceDeblockingTaskThreaded(H264Task *pTask)

bool H264SliceStore_::GetAdditionalDecodingTask(H264Task *pTask)
{
    // this is guarded function, safe to touch any variable

    Ipp32s i;

    // check parallelization condition(s)
    if (m_iSlicesInAdditionalQueue)
    {
        if (m_pAdditionalQueue[0]->IsSliceGroups())
            return false;

        /* DEBUG : CAVLC isn't frame parallelized */
        if (0 == m_pAdditionalQueue[0]->GetPicParam()->entropy_coding_mode)
            return false;
    }

    for (i = 0; i < m_iSlicesInAdditionalQueue; i += 1)
    {
        H264Slice *pSlice = m_pAdditionalQueue[i];

        if ((false == pSlice->m_bDecoded) &&
            (true == pSlice->m_bDecVacant) &&
            (pSlice->m_iCurMBToDec < pSlice->m_iMaxMB))
        {
            Ipp32s iMBWidth = pSlice->GetMBRowWidth();

            pTask->m_bDone = false;
            pTask->m_iFirstMB = pSlice->m_iCurMBToDec;
            pTask->m_iMaxMB = pSlice->m_iMaxMB;
            pTask->m_iMBToProcess = min(pSlice->m_iCurMBToDec -
                                        (pSlice->m_iCurMBToDec % iMBWidth) +
                                        iMBWidth,
                                        pSlice->m_iMaxMB) - pSlice->m_iCurMBToDec;
            pTask->m_iMBToProcess = min(pTask->m_iMBToProcess, pSlice->m_iAvailableMB);
            pTask->m_iTaskID = TASK_DEC_NEXT;
            {
                H264MemoryPiece *pBuffer;
                pBuffer = GetFreeMemoryPiece(m_pFreeBuffers, pTask->m_iMBToProcess * COEFFICIENTS_BUFFER_SIZE * sizeof(Ipp16s));
                if (NULL == pBuffer)
                    return false;
                pTask->m_pBuffer = (Ipp16s *) pBuffer->m_pSourceBuffer;
                AddFilledMemoryPiece(pSlice->m_pAddBuffer, pBuffer);
            }
            pTask->m_pBufferEnd = NULL;
            pTask->m_pSlice = pSlice;
#ifndef DROP_MULTI_THREAD
            pTask->pFunctionMulti = &H264SegmentDecoderMultiThreaded::DecodeSegment;
#endif
			pTask->pFunctionSingle = NULL;

            pSlice->m_bInProcess = true;
            pSlice->m_bDecVacant = false;

#ifdef ECHO
            {
                char cStr[256];
                sprintf(cStr, "(%d) d a - % 4d to % 4d\n",
                    pTask->m_iThreadNumber,
                    pTask->m_iFirstMB,
                    pTask->m_iFirstMB + pTask->m_iMBToProcess);
                OutputDebugString(cStr);
            }
#endif // ECHO

            return true;
        }
    }

    return false;

} // bool H264SliceStore_::GetAdditionalDecodingTask(H264Task *pTask)

#endif

static
void MoveBuffers(Ipp16s **pDst, Ipp16s **pSrc, bool bKeepOrder)
{
    // there is an optimization trick
    // we try to keep hot cache
    if (false == bKeepOrder)
    {

        if (pDst[0])
        {
            Ipp16s *pTemp = pSrc[0];
            Ipp32s i;

            // we insert source buffer into second place of queue.
            // all rest buffers are moved down.
            for (i = 1; i < NUMBER_OF_ROWS; i += 1)
            {
                if (pDst[i])
                {
                    Ipp16s *p;

                    p = pDst[i];
                    pDst[i] = pTemp;
                    pTemp = p;
                }
                else
                {
                    pDst[i] = pTemp;
                    break;
                }
            }
        }
        // there is no any destiantion buffer
        // simply copy top buffer
        else
            pDst[0] = pSrc[0];
    }
    // we need to keep order of moving buffers
    else
    {
        Ipp32s i;

        // find free place
        for (i = 0; i < NUMBER_OF_ROWS; i += 1)
        {
            if (NULL == pDst[i])
                break;
        }

        pDst[i] = pSrc[0];
    }

    // move source buffers
    {
        Ipp32s i;

        for (i = 0; i < NUMBER_OF_ROWS - 1; i += 1)
            pSrc[i] = pSrc[i + 1];
        pSrc[i] = NULL;
    }

} // void MoveBuffers(Ipp16s **pDst, Ipp16s **pSrc, bool bKeepOrder)

#ifndef DROP_MULTI_THREAD_nonono
void H264SliceStore_::AddPerformedTask(H264Task *pTask)
{
#ifndef DROP_MULTI_THREAD
    AutomaticMutex guard(m_mGuard);
#endif
	
	H264Slice *pSlice = pTask->m_pSlice;
 
    // when whole slice was processed
    if (TASK_PROCESS == pTask->m_iTaskID)
    {
        // it is possible only in "slice group" mode
#ifndef DROP_SLICE_GROUP
        if (pTask->m_pSlice->IsSliceGroups())
        {
            pSlice->m_iMaxMB = pTask->m_iMaxMB;
            pSlice->m_iAvailableMB -= pTask->m_iMBToProcess;

            // correct remain uncompressed macroblock count.
            // we can't relay on slice number cause of field pictures.
            if (pSlice->m_iAvailableMB)
            {
                Ipp32s i;

                for (i = 0; i < m_iSlicesInMainQueue; i += 1)
                {
                    if (m_pMainQueue[i] == pSlice)
                        m_pMainQueue[i + 1]->m_iAvailableMB = pSlice->m_iAvailableMB;
                }
            }
        }
#endif

        // slice is decoded
        pSlice->m_bInProcess = false;
        pSlice->m_bDecoded = true;
        pSlice->m_bDecVacant = true;
        pSlice->m_bRecVacant = true;
        pSlice->m_bDebVacant = true;
        // slice is deblocked only when deblocking was available
        if (false == pSlice->IsSliceGroups())
        {
            // check condition for frame deblocking
            if (DEBLOCK_FILTER_ON_NO_SLICE_EDGES == pSlice->m_SliceHeader.disable_deblocking_filter_idc)
                m_bDoFrameDeblocking = false;

            if (false == pSlice->m_bDeblocked)
                pSlice->m_bDeblocked = pSlice->m_bPrevDeblocked;
        }
    }
    else if (TASK_DEB_SLICE == pTask->m_iTaskID)
    {
        pSlice->m_bInProcess = false;
        pSlice->m_bDebVacant = true;
        pSlice->m_bDeblocked = true;
    }
    else if (TASK_DEB_FRAME == pTask->m_iTaskID)
    {
        Ipp32s i;

        // frame is deblocked
        for (i = 0; i < m_iSlicesInMainQueue; i += 1)
        {
            H264Slice *pTemp = m_pMainQueue[i];

            pTemp->m_bInProcess = false;
            pTemp->m_bDebVacant = true;
            pTemp->m_bDeblocked = true;
        }
    }
    else
    {
        switch (pTask->m_iTaskID)
        {
        case TASK_DEC:
            VM_ASSERT(pTask->m_iFirstMB == pSlice->m_iCurMBToDec);

            pSlice->m_iCurMBToDec += pTask->m_iMBToProcess;
            pSlice->m_bDecVacant = true;

            // move filled buffer to reconstruct queue
            MoveBuffers(pSlice->m_pRecBuffer, pSlice->m_pDecBuffer, true);

            break;

        case TASK_REC:
            VM_ASSERT(pTask->m_iFirstMB == pSlice->m_iCurMBToRec);

            pSlice->m_iCurMBToRec += pTask->m_iMBToProcess;
            pSlice->m_bRecVacant = true;
            if (pSlice->m_iMaxMB <= pSlice->m_iCurMBToRec)
            {
                pSlice->m_bDecoded = true;
                // check condition for frame deblocking
                if (DEBLOCK_FILTER_ON_NO_SLICE_EDGES == pSlice->m_SliceHeader.disable_deblocking_filter_idc)
                    m_bDoFrameDeblocking = false;
            }

            if (NULL == pSlice->m_pAddBuffer)
            {
                // move empty buffer to decoding queue
                MoveBuffers(pSlice->m_pDecBuffer, pSlice->m_pRecBuffer, false);
            }
            else
            {
                // move empty buffer to free list
                H264MemoryPiece *pMem = pSlice->m_pAddBuffer;
                pSlice->m_pAddBuffer = pSlice->m_pAddBuffer->m_pNext;
                pMem->m_pNext = m_pFreeBuffers;
                m_pFreeBuffers = pMem;
            }
            break;

        case TASK_DEB:
            VM_ASSERT(pTask->m_iFirstMB == pSlice->m_iCurMBToDeb[0]);

            pSlice->m_iCurMBToDeb += pTask->m_iMBToProcess;
            pSlice->m_bDebVacant = true;
            if (pSlice->m_iMaxMB <= pSlice->m_iCurMBToDeb)
            {
                pSlice->m_bDeblocked = true;
                pSlice->m_bInProcess = false;
            }

            break;

        case TASK_DEB_THREADED:
            VM_ASSERT(pTask->m_iFirstMB == pSlice->m_DebTools.m_iCurMBToDeb[pTask->m_iThreadNumber]);

            pSlice->m_DebTools.SetProcessedMB(pTask);
            if (pSlice->m_DebTools.IsDeblockingDone())
                pSlice->m_bDeblocked = true;

            break;

        case TASK_DEB_FRAME_THREADED:
            m_DebTools.SetProcessedMB(pTask);
            if (m_DebTools.IsDeblockingDone())
            {
                Ipp32s i;

                // frame is deblocked
                for (i = 0; i < m_iSlicesInMainQueue; i += 1)
                {
                    H264Slice *pTemp = m_pMainQueue[i];

                    pTemp->m_bDeblocked = true;
                }
            }

            break;

        case TASK_DEC_NEXT:
            VM_ASSERT(pTask->m_iFirstMB == pSlice->m_iCurMBToDec);

            pSlice->m_iCurMBToDec += pTask->m_iMBToProcess;
            pSlice->m_bDecVacant = true;
            break;

        default:
            // illegal case
            VM_ASSERT(false);
            break;
        }

#ifndef DROP_MULTI_THREAD
        if (m_nWaitingThreads)
        {
            Ipp32s i;
            Ipp32s iMask = 1;

            for (i = 0; i < m_iConsumerNumber; i += 1)
            {
                if (m_nWaitingThreads & iMask)
                    m_eWaiting[i]->Set();
                iMask <<= 1;
            }
        }
#endif
    }

    // there is nothing to do
    // change state of slice store
    SetFrameUncompressed();

} // void H264SliceStore_::AddPerformedTask(H264Task *pTask)
#endif
/*
void H264SliceStore_::AddNextSlice(void)
{
    size_t nUsedSize;
    H264Slice *pPrevSlice;

    // check free slice availability
    if ((NULL == m_pFreeSlices) &&
        (false == AllocateSlice()))
        return;

    {
        H264Bitstream *pBitStream;
        Ipp32u *pSrc, nOffset;

        // find last slice
        pPrevSlice = m_pMainQueue[m_iSlicesInMainQueue - 1];

        VM_ASSERT(pSlice);

        pBitStream = pPrevSlice->GetBitStream();

        pBitStream->GetState(&pSrc, &nOffset);

        nUsedSize = (((Ipp8u *) pSrc) - m_pbSource - 4) & -4;
    }

    // reset slice to initial state
    m_pFreeSlices->m_pPicParamSet = m_pPicParamSet;
    m_pFreeSlices->m_pSeqParamSet = m_pSeqParamSet;
    if ((m_nSize - nUsedSize < MINIMUM_SLICE_LENGTH) ||
        (false == m_pFreeSlices->Reset(m_pbSource + nUsedSize, m_nSize - nUsedSize + 4, 0, true)))
        return;

    // add to list
    if (IsPictureTheSame(pPrevSlice, m_pFreeSlices))
    {
        // fill additional info
        m_pFreeSlices->m_iNumber = pPrevSlice->m_iNumber + 1;
        m_pFreeSlices->m_pCurrentFrame = pPrevSlice->m_pCurrentFrame;
        m_pFreeSlices->m_mbinfo = pPrevSlice->m_mbinfo;
        m_pFreeSlices->m_bNeedToUpdateRefPicList = true;

        m_pFreeSlices->m_iAvailableMB = pPrevSlice->m_iAvailableMB;

        // bind to main queue
        m_pMainQueue[m_iSlicesInMainQueue] = m_pFreeSlices;
        m_iSlicesInMainQueue += 1;
        m_pFreeSlices = m_pFreeSlices->m_pNext;
        m_iSlicesInLastPic += 1;
    }
    else
    {
        // in "unknown mode" frame-level parallelization is prohibited
        VM_ASSERT(0 == m_iSlicesInAdditionalQueue);

        // bind to additional queue
        m_pAdditionalQueue[0] = m_pFreeSlices;
        m_pFreeSlices->SetSliceNumber(GetFirstSliceNumber());
        m_iSlicesInAdditionalQueue += 1;
        m_pFreeSlices = m_pFreeSlices->m_pNext;

        // save last frame info
        m_iAddedFrames += (m_pAdditionalQueue[0]->m_SliceHeader.field_pic_flag) ? (1) : (2);
        m_iSlicesInLastPic = 1;
    }

} // void H264SliceStore_::AddNextSlice(void)
*/
void H264SliceStore_::SetFrameUncompressed(void)
{
    // this is guarded function, safe to touch any variable

    /* DEBUG : there is nothing to do */

} // void H264SliceStore_::SetFrameUncompressed(void)

bool H264SliceStore_::IsFrameUncompressed(void)
{
    // this is guarded function, safe to touch any variable

    return (0 == GetNumberOfSlicesToReconstruct());

} // bool H264SliceStore_::SetFrameUncompressed(void)

bool H264SliceStore_::IsFrameDeblocked(void)
{
    // this is guarded function, safe to touch any variable

    Ipp32s i;

    // there is nothing to do
    if (NULL == m_pMainQueue[0])
        return true;

    for (i = 0; i < m_iSlicesInMainQueue; i += 1)
    {
        H264Slice *pSlice = m_pMainQueue[i];

        if ((pSlice) &&
            (false == pSlice->m_bDeblocked))
            return false;
    }

    return true;

} // bool H264SliceStore_::IsFrameDeblocked(void)

} // namespace UMC
