/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_SPLITTER_H__
#define __UMC_SPLITTER_H__

#include "umc_structures.h"
#include "umc_data_reader.h"
#include "umc_dynamic_cast.h"

namespace UMC
{
class MediaData;

class SplitterParams
{
    DYNAMIC_CAST_DECL_BASE(SplitterParams)

public:
    // Default constructor
    SplitterParams();
    // Destructor
    virtual ~SplitterParams();

    vm_var32 m_lFlags;                                          // (vm_var32) splitter's flags
    DataReader *m_pDataReader;                                  // (DataReader *) pointer to data reader
    vm_var32    m_uiSelectedVideoPID;                           // ID for video stream chosen by user
    vm_var32    m_uiSelectedAudioPID;                           // ID for audio stream chosen by user
};

class SplitterInfo
{
    DYNAMIC_CAST_DECL_BASE(SplitterInfo)

public:
    // Default constructor
    SplitterInfo();

    // Destructor
    virtual ~SplitterInfo();

    // common fields
    vm_var32 m_splitter_flags;

    AudioStreamInfo m_audio_info;                               // (AudioStreamInfo) audio track 0 info
    VideoStreamInfo m_video_info;                               // (VideoStreamInfo) video track 0 info
    SystemStreamInfo m_system_info;                             // (SystemStreamInfo) media stream info

    // memory for auxilary tracks will be allocated inside
    // GetInfo method user should free it then
    AudioStreamInfo *m_audio_info_aux; // auxilary audio tracks 1..
    VideoStreamInfo *m_video_info_aux; // auxilary video tracks 1..

    int number_audio_tracks;                                    // (int) number of available audio tracks
    int number_video_tracks;                                    // (int) number of available video tracks
};

/*
//  Class:       Splitter
//
//  Notes:       Base abstract class of splitter. Class describes
//               the high level interface of abstract splitter of media stream.
//               All specific ( avi, mpeg2, mpeg4 etc ) must be implemented in
//               derevied classes.
//               Splitter uses this class to obtain data
//
*/
class Splitter
{
    DYNAMIC_CAST_DECL_BASE(Splitter)

public:
    Splitter():m_pDataReader(NULL) {}

    // Destructor
    virtual ~Splitter() {}

    // Get media data type
    static SystemStreamType GetStreamType(DataReader *dr);

    // Initialize splitter
    virtual Status Init(SplitterParams& rInit) = 0;

    // Close splitter and free all resources
    virtual Status Close() = 0;

    // Start reading media data
    virtual Status Run() { return UMC_NOT_IMPLEMENTED; }

    // Stop reading media data
    virtual Status Stop() = 0;

    // Get next video data from track
    virtual Status GetNextVideoData(MediaData* data, vm_var32 /*track_idx*/)
    { return GetNextVideoData(data);}

    // Get next audio data from track
    virtual Status GetNextAudioData(MediaData* data, vm_var32 /*track_idx*/)
    { return GetNextAudioData(data);}

    // Get next video data
    virtual Status GetNextVideoData(MediaData* data) = 0;

    // Get next audio data
    virtual Status GetNextAudioData(MediaData* data) = 0;

    // Get next video data
    virtual Status CheckNextVideoData(MediaData* data,vm_var32 track_idx=0)
    {GetNextVideoData(data,track_idx); return UMC_OK;}

    // Get next audio data
    virtual Status CheckNextAudioData(MediaData* data,vm_var32 track_idx=0)
    {GetNextAudioData(data,track_idx); return UMC_OK;}

    // Set position
    virtual Status SetPosition(double pos) = 0;
    virtual Status SetTimePosition(double /*start_time*/)
        {return UMC_NOT_IMPLEMENTED;}

    // Get position
    virtual Status GetPosition(double &pos) = 0;
    virtual Status GetTimePosition(double& pos)
        {pos = 0; return UMC_NOT_IMPLEMENTED;}

    // Get splitter info
    virtual Status GetInfo(SplitterInfo* pInfo) = 0;

    // Set playback rate
    virtual Status SetRate(double /*rate*/) { return UMC_NOT_IMPLEMENTED; }

    virtual Status PrepareForRePosition()
    {return UMC_OK;};
protected:

    DataReader *m_pDataReader;                                  // (DataReader *) pointer to data reader
    SplitterInfo m_info;                                        // (SplitterInfo) splitter info
};

} // namespace UMC

#endif /* __UMC_SPLITTER_H__ */
