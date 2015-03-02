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

#ifndef __UMC_H264_DEC_SLICE_DECODER_H
#define __UMC_H264_DEC_SLICE_DECODER_H

#include "vm_types.h"
#include "umc_structures.h"
#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_dec_slice_store.h"
#include "umc_h264_segment_decoder.h"

namespace UMC
{

class H264SliceStore;

#pragma pack(16)

class H264SliceDecoder
{
public:
    // Default constructor
    H264SliceDecoder(H264SliceStore_ &Store);
    // Destructor
    virtual
    ~H264SliceDecoder(void);

    // Initialize slice decoder
    virtual
    Status Init(Ipp32s iNumber);

    // Start decoding asynchronously
    Status StartProcessing(void);
    // Wait for end of asynchronous decoding
    Status WaitForEndOfProcessing(void);

    // Decode picture segment
    virtual
    Status ProcessSegment(void);
	
	//*** kinoma modification
	void SetApprox( int level ) { m_approx = level; }

protected:
    // Release slice decoder
    void Release(void);

    H264SegmentDecoder *m_pSegmentDecoder;                      // (H264SegmentDecoder *) pointer to segment decoder

    H264SliceStore_ * const m_pSliceStore;                       // (H264SliceStore * const) pointer to slice store object

    Ipp32s m_iNumber;                                           // (Ipp32s) ordinal number of decoder

    //
    // Threading tools
    //

    // Starting routine for decoding thread
    static
    unsigned int DecodingThreadRoutine(void *p);

#ifndef DROP_MULTI_THREAD
    vm_thread m_hThread;                                        // (vm_thread) handle to asynchronously working thread
    vm_event m_hStartProcessing;                                // (vm_event) event to start asynchronous processing
    vm_event m_hDoneProcessing;                                 // (vm_event) event to show end of processing
#endif

    volatile
    bool m_bQuit;                                               // (bool) quit flag for additional thread(s)
    Status m_Status;                                            // (Status) async return value

	//*** kinoma modification
	int	m_approx;
private:
    // we lock assignment operator to avoid any
    // accasionaly assignments
    H264SliceDecoder & operator = (H264SliceDecoder &)
    {
        return *this;

    } // H264SliceDecoder & operator = (H264SliceDecoder &)

};

#pragma pack()

inline
Ipp8s GetReferenceField(Ipp8s *pFields,Ipp8s RefIndex)
{
    if (RefIndex<0)
    {
        return -1;
    }
    else
    {
        VM_ASSERT(pFields[RefIndex]>=0);
        return pFields[RefIndex];
    }

} // Ipp8s GetReferenceField(Ipp8s *pFields,Ipp8s RefIndex)

} // namespace UMC

#endif // __UMC_H264_DEC_SLICE_DECODER_H
