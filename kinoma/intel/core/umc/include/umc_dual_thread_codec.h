/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_DUAL_THREAD_CODEC_H__
#define __UMC_DUAL_THREAD_CODEC_H__

#include "umc_media_data.h"
#include "umc_media_buffer.h"
#include "umc_base_codec.h"

namespace UMC
{

class DualThreadCodecParams
{
public:
    // Default constructor
    DualThreadCodecParams(void);

    MediaBufferParams *m_pMediaBufferParams;                    // (MediaBufferParams *) pointer to media buffer parameter(s)
    MediaBuffer *m_pMediaBuffer;                                // (MediaBuffer *) pointer to media buffer

    BaseCodecParams *m_pCodecInitParams;                        // (BaseCodecParams *) pointer to audio codec parameter(s)
    BaseCodec *m_pCodec;                                        // (BaseCodec *) pointer to audio codec

};

class DualThreadedCodec
{
public:
    // Default constructor
    DualThreadedCodec(void);
    // Destructor
    ~DualThreadedCodec(void);

    // Init dual threaded codec
    Status InitCodec(DualThreadCodecParams *init);

    // Lock input buffer
    Status LockInputBuffer(MediaData *in);

    // Unlock input buffer
    Status UnLockInputBuffer(MediaData *in, Status StreamsStatus = UMC_OK);

    // Get processed audio data
    Status GetFrame(MediaData *out);

    // Close dual threaded codec
    Status Close(void);

    // Stop dual threaded codec
    Status Stop(void);

    // Reset dual threaded codec
    Status Reset(void);

    // Get audio codec info
    Status GetInfo(BaseCodecParams *info);

protected:
    MediaBuffer *m_pMediaBuffer;                                // (MediaBuffer *) pointer to used media buffer
    BaseCodec *m_pCodec;                                        // (BaseCodec *) pointer to used audio codec
};

} // end namespace UMC

#endif /* __UMC_DUAL_THREAD_CODEC_H__ */
