/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_MEDIA_RECEIVER_H__
#define __UMC_MEDIA_RECEIVER_H__

#include "umc_structures.h"
#include "umc_dynamic_cast.h"
#include "umc_media_data.h"

namespace UMC
{
// base class for parameters of renderers and buffers
class  MediaReceiverParams_V51
{
    DYNAMIC_CAST_DECL_BASE(MediaReceiverParams_V51)

public:
    // Default constructor
    MediaReceiverParams_V51(void){}
    // Destructor
    virtual ~MediaReceiverParams_V51(void){}

};

// Base class for renderers and buffers
class MediaReceiver_V51
{
    DYNAMIC_CAST_DECL_BASE(MediaReceiver_V51)

public:
    // Default constructor
    MediaReceiver_V51(void){}
    // Destructor
    virtual ~MediaReceiver_V51(void){}

    // Initialize media receiver
    virtual Status Init(MediaReceiverParams_V51 *init) = 0;

    // Release all receiver resources
    virtual Status Close(void) {return UMC_OK;}

    // Lock input buffer
    virtual Status LockInputBuffer(MediaData_V51 *in) = 0;
    // Unlock input buffer
    virtual Status UnLockInputBuffer(MediaData_V51 *in, Status StreamStatus = UMC_OK) = 0;

    // Break waiting(s)
    virtual Status Stop(void) = 0;

    // Reset media receiver
    virtual Status Reset(void) {return UMC_OK;}

};

} // end namespace UMC


#endif // __UMC_MEDIA_RECEIVER_H__
