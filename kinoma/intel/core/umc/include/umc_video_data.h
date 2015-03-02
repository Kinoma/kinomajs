/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_VIDEO_DATA_H__
#define __UMC_VIDEO_DATA_H__

#include "vm_event.h"
#include "umc_structures.h"
#include "umc_media_data.h"

namespace UMC
{

enum
{
    NUMBER_OF_PLANES            = 3
};

class VideoData_V51 : public MediaData_V51
{
    DYNAMIC_CAST_DECL(VideoData_V51, MediaData_V51)

public:

    // Default constructor
    VideoData_V51();
    // Destructor
    virtual ~VideoData_V51();

    // Initiliaze. Function also sets pitches, and pitches can be
    // larger than width because of aligning.
    // pData can be either NULL or a pointer to buffer for video data
    // with the size of at least GetMappingSize(width, height, cFormat) bytes.
    virtual Status Init(vm_var32 width,
                        vm_var32 height,
                        ColorFormat cFormat,
                        vm_byte  *pData = NULL);

    // Allocate buffer for video data and initialize.
    // Size of allocated buffer is equal to
    // GetMappingSize(width, height, cFormat),
    // it will be released in destructor or Close().
    virtual Status InitAlloc(vm_var32 width,
                             vm_var32 height,
                             ColorFormat cFormat);

    // Release video data
    virtual Status Close(void);

    ////////////////////////////////////////////////////////////

    // Sets buffer pointer as well as initializes m_lpImageDest[]
    // array of pointers.
    virtual Status SetBufferPointer(vm_byte *ptr, size_t bytes);

    // Set destination pointer(s)
    Status SetDest(void *pDest0, void *pDest1 = NULL, void *pDest2 = NULL);

    // Set image pitch(es)
    Status SetPitch(size_t lPitch0, size_t lPitch1 = 0, size_t lPitch2 = 0);

    // Set image width, height, color format.
    virtual Status SetVideoParameters(vm_var32 width,
                                      vm_var32 height,
                                      ColorFormat cFormat);

    // Set color format
    Status SetColorFormat(ColorFormat cFormat);

    // Set frame type
    Status SetFrameType(FrameType ft);

    inline VideoData_V51 &operator = (VideoData_V51 &in);

    ////////////////////////////////////////////////////////////

    // Returns the needed size of a buffer for mapping.
    static size_t GetMappingSize(vm_var32 width,
                                 vm_var32 height,
                                 ColorFormat cFormat);

    // Returns mapping size of the images.
    // Equal to GetMappingSize()
    static size_t GetImagesSize(vm_var32 width,
                                vm_var32 height,
                                ColorFormat cFormat);

    // Set image width, height, color format and pitch.
    virtual Status GetVideoParameters(vm_var32& width,
                                      vm_var32& height,
                                      ColorFormat& cFormat);

    vm_byte * ((&m_lpDest)[3]);               // (vm_byte * const ([])) public use pointer(s) to picture
    const size_t (&m_lPitch)[3];                    // (const size_t []) public use pitch(es)
    const FrameType &m_FrameType;                   // (const FrameType) public use frame type
    const ColorFormat &m_ColorFormat;               // (color ColorFormat) public use color format of image

protected:
    vm_byte *(m_lpImageDest[NUMBER_OF_PLANES]);                 // (vm_byte *([])) pointer(s) to data

    size_t m_lImagePitch[3];                        // (size_t []) image pitch(es)
    FrameType m_ImageFrameType;                     // (FrameType) type of frame
    ColorFormat m_ImageColorFormat;                 // (ColorFormat) color format of image
    vm_var32 m_Width;                               // (vm_var32) width of the frame.
    vm_var32 m_Height;                              // (vm_var32) height of the frame.
};

class VideoDataSync_V51 : public VideoData_V51
{
    DYNAMIC_CAST_DECL(VideoDataSync_V51, VideoData_V51)

public:
    VideoDataSync_V51():m_pEvent(NULL){};

    // Set pointer to synchro tool
    inline void SetSync(vm_event *pEvent);

    // Get pointer to synchro tool
    inline void GetSync(vm_event *(&pEvent));

    // unlock image
    inline void UnlockImage(void);

    inline VideoDataSync_V51& operator = (VideoDataSync_V51& in);

protected:
    vm_event *m_pEvent;   // (vm_event) use protection tool

};

inline
Status VideoData_V51::SetDest(void *pDest0, void *pDest1, void *pDest2)
{
    m_lpImageDest[0] = reinterpret_cast<vm_byte *> (pDest0);
    m_lpImageDest[1] = reinterpret_cast<vm_byte *> (pDest1);
    m_lpImageDest[2] = reinterpret_cast<vm_byte *> (pDest2);

    return UMC_OK;

} // Status VideoData_V51::SetDest(void *pDest0, void *pDest1, void *pDest2)

inline
Status VideoData_V51::SetPitch(size_t lPitch0, size_t lPitch1, size_t lPitch2)
{
    m_lImagePitch[0] = lPitch0;
    m_lImagePitch[1] = lPitch1;
    m_lImagePitch[2] = lPitch2;

    return UMC_OK;

} // Status VideoData_V51::SetPitch(size_t lPitch0, size_t lPitch1, size_t lPitch2)

inline
Status VideoData_V51::SetFrameType(FrameType ft)
{
    m_ImageFrameType = ft;

    return UMC_OK;

} // Status VideoData_V51::SetFrameType(FrameType ft)

inline
Status VideoData_V51::SetColorFormat(ColorFormat cFormat)
{
    m_ImageColorFormat = cFormat;

    return UMC_OK;

} // Status VideoData_V51::SetColorFormat(ColorFormat cFormat)

inline
VideoData_V51 &VideoData_V51::operator = (VideoData_V51 &in)
{
    MediaData_V51::operator=(in);
    m_ImageColorFormat = in.m_ImageColorFormat;
    m_ImageFrameType   = in.m_ImageFrameType;
    m_lImagePitch[0]   = in.m_lImagePitch[0];
    m_lImagePitch[1]   = in.m_lImagePitch[1];
    m_lImagePitch[2]   = in.m_lImagePitch[2];
    m_lpImageDest[0]   = in.m_lpImageDest[0];
    m_lpImageDest[1]   = in.m_lpImageDest[1];
    m_lpImageDest[2]   = in.m_lpImageDest[2];
    m_Width            = in.m_Width;
    m_Height           = in.m_Height;

    return *this;

} // VideoData_V51 &VideoData_V51::operator = (VideoData_V51 &in)

inline
void VideoDataSync_V51::SetSync(vm_event *pEvent)
{
    m_pEvent = pEvent;

} // void VideoDataSync_V51::SetSync(vm_event *pEvent)

inline
void VideoDataSync_V51::GetSync(vm_event *(&pEvent))
{
    pEvent = m_pEvent;

} // void VideoDataSync_V51::GetSync(vm_event *(&pEvent))

inline
void VideoDataSync_V51::UnlockImage(void)
{
    if (m_pEvent)
        vm_event_signal(m_pEvent);

} // void VideoDataSync_V51::UnlockImage(void)

inline
VideoDataSync_V51 &VideoDataSync_V51::operator = (VideoDataSync_V51 &in)
{
    VideoData_V51::operator = (in);
    m_pEvent = in.m_pEvent;

    return *this;

} // VideoDataSync_V51 &VideoDataSync_V51::operator = (VideoDataSync_V51 &in)

} // end namespace UMC

#endif // __UMC_VIDEO_DATA_H__
