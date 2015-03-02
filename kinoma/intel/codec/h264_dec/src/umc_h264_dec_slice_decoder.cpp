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

#include "umc_h264_dec_slice_decoder.h"
#include "umc_h264_segment_decoder.h"
#include "umc_h264_bitstream.h"
#include "umc_h264_dec_slice_store.h"
#include "vm_thread.h"
#include "vm_event.h"

namespace UMC
{

H264SliceDecoder::H264SliceDecoder(H264SliceStore_ &Store) :
    m_pSliceStore(&Store)
{
    m_pSegmentDecoder = NULL;

#ifndef DROP_MULTI_THREAD
    vm_thread_set_invalid(&m_hThread);
    vm_event_set_invalid(&m_hStartProcessing);
    vm_event_set_invalid(&m_hDoneProcessing);
#endif

    m_bQuit = false;

	//*** kinoma modification
	m_approx = 0;

} // H264SliceDecoder::H264SliceDecoder(H264SliceStore_ &Store) :

H264SliceDecoder::~H264SliceDecoder(void)
{
    Release();

} // H264SliceDecoder::~H264SliceDecoder(void)

void H264SliceDecoder::Release(void)
{
    if (m_pSegmentDecoder)
        delete m_pSegmentDecoder;
    m_pSegmentDecoder = NULL;

    // threading tools
#ifndef DROP_MULTI_THREAD
    if (vm_thread_is_valid(&m_hThread))
    {
        m_bQuit = true;
        vm_event_signal(&m_hStartProcessing);

        vm_thread_wait(&m_hThread);
        vm_thread_close(&m_hThread);
    }

    vm_thread_set_invalid(&m_hThread);
    vm_event_set_invalid(&m_hStartProcessing);
    vm_event_set_invalid(&m_hDoneProcessing);
#endif

    m_bQuit = false;

} // void H264SliceDecoder::Release(void)

Status H264SliceDecoder::Init(Ipp32s iNumber)
{
#ifndef DROP_MULTI_THREAD
    vm_status vmRes;
#endif

    // release object before initialization
    Release();

    // save thread number(s)
    m_iNumber = iNumber;

    // create segment decoder
    m_pSegmentDecoder = new H264SegmentDecoder(*m_pSliceStore);
    if (NULL == m_pSegmentDecoder)
        return UMC_ALLOC;
    if (UMC_OK != m_pSegmentDecoder->Init(m_iNumber))
        return UMC_FAILED_TO_INITIALIZE;

    // threading tools
    m_bQuit = false;

#ifndef DROP_MULTI_THREAD
    if (iNumber)
    {
        // initialize working events
        vmRes = vm_event_init(&m_hStartProcessing, 0, 0);
        if (VM_OK != vmRes)
            return UMC_FAILED_TO_INITIALIZE;
        vmRes = vm_event_init(&m_hDoneProcessing, 0, 0);
        if (VM_OK != vmRes)
            return UMC_FAILED_TO_INITIALIZE;

        // start decoding thread
        {
            unsigned int res;
            res = vm_thread_create(&m_hThread, DecodingThreadRoutine, this);
            if (0 == res)
                return UMC_FAILED_TO_INITIALIZE;
        }
    }
#endif

    return UMC_OK;

} // Status H264SliceDecoder::Init(Ipp32u nNumber)

Status H264SliceDecoder::StartProcessing(void)
{
#ifndef DROP_MULTI_THREAD
    if (0 == vm_event_is_valid(&m_hStartProcessing))
        return UMC_OPERATION_FAILED;

    vm_event_signal(&m_hStartProcessing);
#endif

    return UMC_OK;

} // Status H264SliceDecoder::StartProc(void)

Status H264SliceDecoder::WaitForEndOfProcessing(void)
{
    Status res;

#ifndef DROP_MULTI_THREAD
    if (0 == vm_event_is_valid(&m_hDoneProcessing))
        return UMC_OPERATION_FAILED;

    vm_event_wait(&m_hDoneProcessing);
#endif

    res = m_Status;

    return res;

} // Status H264SliceDecoder::WaitForEndDecoding(void)

Status H264SliceDecoder::ProcessSegment(void)
{
    m_pSegmentDecoder->SetApprox(m_approx);
	return m_pSegmentDecoder->ProcessSegment();

} // Status H264SliceDecoder::ProcessSegment(void)


#ifndef DROP_MULTI_THREAD
unsigned int H264SliceDecoder::DecodingThreadRoutine(void *p)
{
    H264SliceDecoder *pObj = (H264SliceDecoder *) p;

    // check error(s)
    if (NULL == p)
        return 0x0baad;

    {
        vm_event (&hStartProcessing)(pObj->m_hStartProcessing);
        vm_event (&hDoneProcessing)(pObj->m_hDoneProcessing);

        // wait for begin decoding
        vm_event_wait(&hStartProcessing);

        while (false == pObj->m_bQuit)
        {
            // do segment decoding
            pObj->m_Status = pObj->ProcessSegment();

            // set done event
            vm_event_signal(&hDoneProcessing);

            // wait for begin decoding
            vm_event_wait(&hStartProcessing);
        }
    }

    return 0x0264dec0 + pObj->m_iNumber;

} // unsigned int H264SliceDecoder::DecodingThreadRoutine(void *p)
#endif

} // namespace UMC
