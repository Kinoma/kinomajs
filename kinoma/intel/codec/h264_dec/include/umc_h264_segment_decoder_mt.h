/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//    Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/

#ifndef __UMC_H264_SEGMENT_DECODER_MT_H
#define __UMC_H264_SEGMENT_DECODER_MT_H

#include "umc_h264_segment_decoder.h"

namespace UMC
{

#pragma pack(16)

class H264SegmentDecoderMultiThreaded : public H264SegmentDecoder
{
public:
    // Default constructor
    H264SegmentDecoderMultiThreaded(H264SliceStore_ &Store);
    // Destructor
    virtual
    ~H264SegmentDecoderMultiThreaded(void);

    // Initialize object
    virtual
    Status Init(Ipp32s iNumber);

    // Decode slice's segment
    virtual
    Status ProcessSegment(void);

    // asynchronous called functions
    Status DecodeSegment(Ipp32s iCurMBNumber, Ipp32s &iMBToDecode);
    Status ReconstructSegment(Ipp32s iCurMBNumber, Ipp32s &iMBToReconstruct);
    Status DeblockSegment(Ipp32s iCurMBNumber, Ipp32s &iMBToDeblock);

protected:
    // Release object
    void Release(void);

    Ipp16s *m_psBuffer;
    Ipp16s *GetCoefficientsBuffer(void)
    {
        return m_psBuffer;
    }

    // Allocated more coefficients buffers
    Status ReallocateCoefficientsBuffer(void);

    Status DecodeMacroBlockCAVLC(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCAVLC_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCAVLC_H0(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCAVLC_H0_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCAVLC_H2(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCAVLC_H2_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCAVLC_H4(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCAVLC_H4_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCABAC(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCABAC_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    Status DecodeMacroBlockCABAC_H0(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCABAC_H0_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCABAC_H2(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCABAC_H2_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCABAC_H4(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status DecodeMacroBlockCABAC_H4_FLD(Ipp32u nCurMBNumber, Ipp32u &nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    Status ReconstructMacroBlockCAVLC(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCAVLC_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    Status ReconstructMacroBlockCAVLC_H0(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCAVLC_H0_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCAVLC_H2(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCAVLC_H2_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCAVLC_H4(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCAVLC_H4_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    Status ReconstructMacroBlockCABAC(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCABAC_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    Status ReconstructMacroBlockCABAC_H0(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCABAC_H0_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCABAC_H2(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCABAC_H2_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCABAC_H4(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status ReconstructMacroBlockCABAC_H4_FLD(Ipp32u nCurMBNumber, Ipp32u nMaxMBNumber, H264VideoDecoder::H264DecoderFrame **pRefPicList0, H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    // Get direct motion vectors for block 4x4
    Status GetMVD4x4_CABAC(const Ipp8u *pBlkIdx,
                           const Ipp8u *pCodMVd,
                           Ipp32u ListNum);
    Status GetMVD4x4_16x8_CABAC(const Ipp8u *pCodMVd,
                                Ipp32u ListNum);
    Status GetMVD4x4_8x16_CABAC(const Ipp8u *pCodMVd,
                                Ipp32u ListNum);
    Status GetMVD4x4_CABAC(const Ipp8u pCodMVd,
                           Ipp32u ListNum);

    // Decode skipped motion vectors
    bool DecodeSkipMotionVectors();
    // Decode skipped motion vectors
    bool ReconstructSkipMotionVectors();
    // Decode motion vectors
/*
    Status DecodeMotionVectors(bool bIsBSlice);*/
    Status DecodeMotionVectors_CABAC();
    // Decode motion vectors
    Status DecodeDirectMotionVectorsSpatial(void);
    // Reconstruct motion vectors
    Status ReconstructMotionVectors(void);
    // Decode motion vectors
    Status ReconstructDirectMotionVectorsSpatial(H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                                 H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                                 Ipp8u Field,
                                                 bool bUseDirect8x8Inference);
    Status ReconstructMotionVectors4x4(const Ipp8u pCodMVd,
                                       Ipp32u ListNum);
    Status ReconstructMotionVectors4x4(const Ipp8u *pBlkIdx,
                                       const Ipp8u* pCodMVd,
                                       Ipp32u ListNum);
    Status ReconstructMotionVectors16x8(const Ipp8u *pCodMVd,
                                        Ipp32u ListNum);
    Status ReconstructMotionVectors8x16(const Ipp8u *pCodMVd,
                                        Ipp32u ListNum);
    // Do slice deblocking
    void DeblockSegment(void);

    Ipp32u m_nNumberOfDecodersPerSlice;

    // pointer(s) to current decoding function
    Status (H264SegmentDecoderMultiThreaded::*pDecodeMacroBlockRow)(Ipp32u nCurMBNumber,
                                                                 Ipp32u &nMaxMBNumber,
                                                                 H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                                                 H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    Status (H264SegmentDecoderMultiThreaded::*pReconstructMacroBlockRow)(Ipp32u nCurMBNumber,
                                                                      Ipp32u nMaxMBNumber,
                                                                      H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                                                      H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    Ipp32u m_nLeftDecodingMB;                                   // (Ipp32u) number of left decoding macroblock in row
    Ipp32u m_nRightDecodingMB;                                  // (Ipp32u) number of right decoding macroblock in row
    Ipp32u m_nLeftDeblockingMB;                                 // (Ipp32u) number of left deblocking macroblock in row
    Ipp32u m_nRightDeblockingMB;                                // (Ipp32u) number of right deblocking macroblock in row

    vm_event *m_pReadyDecoding;
    vm_event *m_pReadyReconstruction;
    vm_semaphore *m_pReadyDeblocking;

    vm_event *m_pDoneLeft;
    vm_event *m_pDoneRight;

    //
    // Threading tools
    //

    H264SegmentStore *m_pSegmentStore;

    // Starting routine for decoding thread
    static
    unsigned int DecodingThreadRoutine(void *p);
    static
    unsigned int DeblockingThreadRoutine(void *p);

    vm_thread m_hThreadDecoding;                                // (vm_thread) handle to asynchronously working thread
    vm_thread m_hThreadDeblocking;                              // (vm_thread) handle to asynchronously working thread

    vm_event m_hStartDecoding;                                  // (vm_event) event to start asynchronous processing
    vm_event m_hDoneDecoding;                                   // (vm_event) event to show end of processing

    vm_event m_hStartDeblocking;                                // (vm_event) event to start asynchronous processing
    vm_event m_hDoneDeblocking;                                 // (vm_event) event to show end of processing

    volatile
    bool m_bQuit;                                               // (bool) quit flag for additional thread(s)
    Status m_Status;                                            // (Status) async return value

};

#pragma pack()

} // namespace UMC

#endif /* __UMC_H264_SEGMENT_DECODER_MT_H */
