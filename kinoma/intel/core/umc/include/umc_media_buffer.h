/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_MEDIA_BUFFER_H__
#define __UMC_MEDIA_BUFFER_H__

#include "umc_structures.h"
#include "umc_dynamic_cast.h"
#include "umc_media_receiver.h"

namespace UMC
{
class MediaBufferParams_V51 : public MediaReceiverParams_V51
{
    DYNAMIC_CAST_DECL(MediaReceiverParams_V51, MediaReceiverParams_V51)

public:
    // Default constructor
    MediaReceiverParams_V51(void) :
        m_prefInputBufferSize(0),
        m_numberOfFrames(0),
        m_prefOutputBufferSize(0)
    {}

    size_t m_prefInputBufferSize;                               // (size_t) preferable size of input potion(s)
    vm_var32 m_numberOfFrames;                                  // (vm_var32) minimum number of data potion in buffer
    size_t m_prefOutputBufferSize;                              // (size_t) preferable size of output potion(s)
};

class MediaBuffer_V51 : public MediaReceiver_V51
{
    DYNAMIC_CAST_DECL(MediaBuffer_V51, MediaReceiver_V51)

public:

    // Constructor
    MediaBuffer_V51(void) { m_bDirty = false; }
    // Destructor
    virtual ~MediaBuffer_V51(void){}

    // Lock output buffer
    virtual Status LockOutputBuffer(MediaData_V51 *out) = 0;
    // Unlock output buffer
    virtual Status UnLockOutputBuffer(MediaData_V51 *out) = 0;

    bool     m_bDirty;                                           // (bool) is the first received data should be ignored
};

} // end namespace UMC

#endif /* __UMC_MEDIA_BUFFER_H__ */
