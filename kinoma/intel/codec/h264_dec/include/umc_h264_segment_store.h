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

#ifndef __UMC_H264_SEGMENT_STORE_H
#define __UMC_H264_SEGMENT_STORE_H

#include "umc_structures.h"
#include "vm_mutex.h"
#include "ippi.h"

namespace UMC
{

// forward declaration of used types
struct H264SliceHeader;
class H264SegmentDecoderMultiThreaded;
class AutomaticMutex;

#pragma pack(16)

class H264SegmentTask
{
public:
    H264SegmentTask(Ipp32s iThreadNumber)
    {
        m_iThreadNumber = iThreadNumber;
        m_bDone = false;
    }

    Status (H264SegmentDecoderMultiThreaded::*pFunction)(Ipp32s nCurMBNumber, Ipp32s &nMaxMBNumber); // (Status (*) (Ipp32s, Ipp32s &)) pointer to working function
    Ipp16s *m_pBuffer;                                          // (Ipp16s *) pointer to temorary working buffer

    Ipp32s m_iThreadNumber;                                     // (Ipp32s) number of current thread
    Ipp32s m_iTaskID;                                           // (Ipp32s) task ID

    Ipp32s m_iCurMBNumber;                                      // (Ipp32s) first MB number of task
    Ipp32s m_iMBToProcess;                                      // (Ipp32s) MB count to process
    Ipp32s m_iMaxMBNumber;                                      // (Ipp32s) max MB number of task

    bool m_bDone;                                               // (bool) task was performed
};

#pragma pack()
/*
enum
{
    NUMBER_OF_ROWS              = 4,
    NUMBER_OF_PIECES            = 8
};
*/
class H264SegmentStore
{
public:
    // Default constructor
    H264SegmentStore(void);
    // Destructor
    virtual
    ~H264SegmentStore(void);

    // Initialize segment store
    bool Initialize(Ipp32s iFirstMB, Ipp32s iMaxMB, Ipp32s iMBWidth, bool bMBAFF, bool bDoDeblocking, bool bOnlyDeblocking = false);

    // Get new task
    bool GetNewTask(H264SegmentTask *pTask);

protected:
    // Release object
    void Release(void);

    // Add processed task
    void AddPerformedTask(H264SegmentTask *pTask);
    // Try to obtain new decoding task
    bool GetNewDecTask(H264SegmentTask *pTask);
    // Try to obtain new reconstruction task
    bool GetNewRecTask(H264SegmentTask *pTask);
    // Try to obtain new deblocking task
    bool GetNewDebTask(H264SegmentTask *pTask);
    // Try to obtain new deblocking task (multithreaded version)
    bool GetNewDebTaskThreaded(H264SegmentTask *pTask, AutomaticMutex &guard);

    //
    // there are only inter-thread variables
    //

    volatile
    bool m_bQuit;                                               // (bool) quit flag

    Ipp16s *m_pAllocated;                                       // (Ipp16s *) pointer to allocated temporary buffer
    Ipp32s m_iAllocatedMB;                                      // (Ipp32s) size of allocated buffer
    Ipp32s m_iDecodersNumber;                                   // (Ipp32s) number of decoders

    volatile
    Ipp32s m_iCurMBToDec;                                       // (Ipp32s) number of first MB to decode
    volatile
    Ipp32s m_iCurMBToRec;                                       // (Ipp32s) number of first MB to reconstruct
    volatile
    Ipp32s m_iCurMBToDeb[2];                                    // (Ipp32s) number of first MB to de-blocking
    volatile
    Ipp32s m_iMaxMBNumber;                                      // (Ipp32s) maximum number of MB
    volatile
    Ipp32s m_iMBWidth;                                          // (Ipp32s) MB row size

    bool m_bDoDeblocking;                                       // (bool) deblocking condition
    bool m_bDeblockingVacant;                                   // (bool) deblocking is vacant
    Ipp16s *(m_pDecBuffer[NUMBER_OF_ROWS]);                     // (Ipp16s *[]) array of pointers to working buffers for decoding
    Ipp16s *(m_pRecBuffer[NUMBER_OF_ROWS]);                     // (Ipp16s *[]) array of pointers to working buffers for reconstruction

    volatile
    Ipp32s m_iWaitingThreads;                                   // (Ipp32s) number of waiting threads

    vm_mutex m_mGuard;                                          // (vm_mutex) synchro tool
    vm_event m_eWaiting;                                        // (vm_event) waiting list event

    volatile
    bool m_bFirstDebThreadedCall;                               // (bool) we need reset event(s) at first call

    Ipp32s m_iDebUnit;                                          // (Ipp32s) minimal deblocking unit in macroblocks

    vm_event m_eLeftReady;                                      // (vm_event) array of left decoding events
    vm_event m_eRightReady;                                     // (vm_event) array of right decoding events
};

} // namespace UMC

#endif // __UMC_H264_SEGMENT_STORE_H
