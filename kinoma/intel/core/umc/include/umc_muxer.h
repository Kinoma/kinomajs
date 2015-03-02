/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_MUXER_H__
#define __UMC_MUXER_H__

#include "umc_media_data.h"
#include "umc_structures.h"

namespace UMC
{

// forward declaration(s)
class DataWriter;

class MuxerParams
{
    DYNAMIC_CAST_DECL_BASE(MuxerParams)

public:
    // Default constructor
    MuxerParams()
    {
        m_SystemType = UNDEF_STREAM;
        m_lFlags = 0;
        m_lChunkSizeLimit = 0;
        m_lpDataWriter = NULL;
    }
    // Destructor
    virtual ~MuxerParams(){}

    SystemStreamType m_SystemType;                              // (SystemStreamType) subtype of media stream
    vm_var32 m_lFlags;                                          // (vm_var32) muxer flag(s)
    vm_var32 m_lChunkSizeLimit;                                 // (vm_var32) max size of data chunk
    vm_var32 m_nNumberOfAudioStreams;                           // (vm_var32) number of audio streams
    vm_var32 m_nNumberOfVideoStreams;                           // (vm_var32) number of video streams
    AudioStreamInfo *m_lpAudioInfo;                             // (AudioStreamInfo *) pointer to audio stream info(s)
    VideoStreamInfo *m_lpVideoInfo;                             // (AudioStreamInfo *) pointer to video stream info(s)

    DataWriter *m_lpDataWriter;                                 // (DataWriter *) pointer to using data writer

};

class MuxerInfo
{
    DYNAMIC_CAST_DECL_BASE(MuxerInfo)

public:
    // Default constructor
    MuxerInfo(void)
    {
        m_lMuxerFlags = 0;
        memset(&m_AudioInfo, 0, sizeof(m_AudioInfo));
        memset(&m_VideoInfo, 0, sizeof(m_VideoInfo));
        memset(&m_SystemInfo, 0, sizeof(m_SystemInfo));
    }

    // Destructor
    virtual ~MuxerInfo(void){}

    vm_var32 m_lMuxerFlags;                                     // (vm_var32) muxer flag(s)
    AudioStreamInfo m_AudioInfo;                                // (AudioStreamInfo) encoded audio stream info
    VideoStreamInfo m_VideoInfo;                                // (VideoStreamInfo) encoded video stream info
    SystemStreamInfo m_SystemInfo;                              // (SystemStreamInfo) resulted stream info

};

enum MuxerProcessStatus
{
    PROCESSING_NOT_STARTED      = -1,
    PROCESSING_FINISHED         = 0,
    PROCESSING_IN_PROGRESS      = 1
};

class Muxer
{
    DYNAMIC_CAST_DECL_BASE(Muxer)

public:
    // Default constructor
    Muxer()
    {
        m_lpDataWriter = NULL;
        m_lInternalProcStatus = -1;
    }

    // Destructor
    virtual ~Muxer(){}

    // Get stream type
    static SystemStreamType GetStreamType(DataWriter * /* lpWriter */)
    { return MPEG2_PROGRAMM_STREAM;}

    // Close muxer and release all resources
    virtual Status Close(void) = 0;

    // Initialize muxer
    virtual Status Init(MuxerParams *lpInit) = 0;

    // Add new video potion
    virtual Status PutVideoData(MediaData *lpData, vm_var32 nStreamNumber, Status StreamStatus = UMC_OK) = 0;

    // Add new audio potion
    virtual Status PutAudioData(MediaData *lpData, vm_var32 nStreamNumber, Status StreamStatus = UMC_OK) = 0;

    // Get muxer info
    virtual Status GetInfo(MuxerInfo *lpInfo) = 0;

    //must return MuxerProcessStatus value
    virtual Status GetMuxerStatus(vm_var32 & /*sts*/) { return UMC::UMC_OK; };


protected:

    DataWriter *m_lpDataWriter;                                 // (DataWriter *) pointer to using data writer
    MuxerInfo m_Info;                                           // (MuxerInfo) current muxer info
    signed int m_lInternalProcStatus;                           // UTA
};

} // end namespace UMC

#endif // __UMC_MUXER_H__
