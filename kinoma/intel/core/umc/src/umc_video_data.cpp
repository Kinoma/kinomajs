/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "umc_video_data.h"

namespace UMC
{

enum
{
    ALIGN_VALUE                 = 128
};

VideoData_V51::VideoData_V51(void) :
    m_lpDest(m_lpImageDest),
    m_lPitch(m_lImagePitch),
    m_FrameType(m_ImageFrameType),
    m_ColorFormat(m_ImageColorFormat)
{
    m_lpImageDest[0] =
    m_lpImageDest[1] =
    m_lpImageDest[2] = NULL;

    m_lImagePitch[0] =
    m_lImagePitch[1] =
    m_lImagePitch[2] = 0;

    m_ImageFrameType = NONE_PICTURE;

    m_ImageColorFormat = NONE;
    m_Width = 0;
    m_Height = 0;

} // VideoData_V51::VideoData_V51(void)

VideoData_V51::~VideoData_V51(void)
{
    Close();

} // VideoData_V51::~VideoData_V51(void)

Status VideoData_V51::Close(void)
{
    return MediaData_V51::Close();

} // Status VideoData_V51::Close(void)

size_t VideoData_V51::GetImagesSize(vm_var32 width,
                                vm_var32 height,
                                ColorFormat cFormat)
{
    size_t size = (unsigned) -1;
    size_t nPitch = 0;

    switch(cFormat)
    {
    case YV12:
    case NV12:
    case YUV420:
        nPitch =  align_value<size_t> (width, ALIGN_VALUE);
        size = (nPitch * height * 3) / 2;
        break;

    case YUV422:
        nPitch =  align_value<size_t> (width, ALIGN_VALUE);
        size = nPitch * height * 2;
        break;

    case YUV444:
        nPitch =  align_value<size_t> (width, ALIGN_VALUE);
        size = nPitch * height * 4;
        break;

    case YUY2:
    case UYVY:
    case RGB565:
    case RGB555:
    case RGB444:
        nPitch = align_value<size_t> (width * 2, ALIGN_VALUE);
        size = nPitch * height;
        break;

    case RGB24:
        nPitch =  align_value<size_t> (width * 3, ALIGN_VALUE);
        size = nPitch * height;
        break;

    case RGB32:
        nPitch =  align_value<size_t> (width * 4, ALIGN_VALUE);
        size = nPitch * height;
        break;

        // bad image format
    default:
        return size;
    }

    return size;

} // size_t VideoData_V51::GetImagesSize(vm_var32 width,

size_t VideoData_V51::GetMappingSize(vm_var32 width,
                                 vm_var32 height,
                                 ColorFormat cFormat)
{
    size_t size = GetImagesSize(width, height, cFormat);
    return size;

} // size_t VideoData_V51::GetMappingSize(vm_var32 width,

Status VideoData_V51::GetVideoParameters(vm_var32& width,
                                     vm_var32& height,
                                     ColorFormat& cFormat)
{
    width  = m_Width;
    height = m_Height;
    cFormat = m_ImageColorFormat;

    return UMC_OK;
}

Status VideoData_V51::SetVideoParameters(vm_var32 width,
                                     vm_var32 height,
                                     ColorFormat cFormat)
{

    m_Width = width;
    m_Height = height;
    m_ImageColorFormat = cFormat;

    return UMC_OK;
}

Status VideoData_V51::Init(vm_var32 width,
                       vm_var32 height,
                       ColorFormat cFormat,
                       vm_byte  *pData)
{
    Status res;
    size_t nPitch = 0;

    switch(cFormat)
    {
    case YV12:
    case NV12:
    case YUV420:
    case YUV422:
    case YUV444:
        nPitch =  align_value<size_t> (width, ALIGN_VALUE);
        break;

    case YUY2:
    case UYVY:
    case RGB565:
    case RGB555:
    case RGB444:
        nPitch = align_value<size_t> (width * 2, ALIGN_VALUE);
        break;

    case RGB24:
        nPitch =  align_value<size_t> (width * 3, ALIGN_VALUE);
        break;

    case RGB32:
        nPitch =  align_value<size_t> (width * 4, ALIGN_VALUE);
        break;

        // bad image format
    default:
        return(UMC_BAD_FORMAT);
    }

    switch (cFormat)
    {
    case YV12:
    case YUV420:
    case YUV422:
        SetPitch(nPitch, nPitch / 2, nPitch / 2);
        break;

    case YUV444:
        SetPitch(nPitch, nPitch, nPitch);
        break;

    case NV12:
        SetPitch(nPitch, nPitch, 0);
        break;

    default:
        SetPitch(nPitch);
        break;
    };

    res = SetVideoParameters(width, height, cFormat);
    if (UMC_OK != res)
        return res;

    if (NULL == pData) {
      m_lpImageDest[0] = m_lpImageDest[1] = m_lpImageDest[2] = NULL;
      return UMC_OK;
    } else {
      return SetBufferPointer(pData, GetMappingSize(width, height, cFormat));
    }
} // Status VideoData_V51::Init

Status VideoData_V51::InitAlloc(vm_var32 width,
                            vm_var32 height,
                            ColorFormat cFormat)
{
    Status res;
    size_t size;
    vm_byte *pData;

    Close(); // release memory

    size = GetMappingSize(width, height, cFormat);
    if (size <= 0) {
        return UMC_FAILED_TO_ALLOCATE_BUFFER;
    }
    pData = new vm_byte[size];
    if (NULL == pData) {
        return UMC_FAILED_TO_ALLOCATE_BUFFER;
    }

    res = Init(width, height, cFormat, pData); // MediaData_V51::Close() inside!

    m_bMemoryAllocated = 1; // Buffer pointer was allocated

    return res;
}

Status VideoData_V51::SetBufferPointer(vm_byte *ptr, size_t bytes)
{
    Status res = MediaData_V51::SetBufferPointer((vm_byte *)ptr, bytes);

    if (res != UMC_OK)
        return res;

    switch (m_ImageColorFormat)
    {
    case YV12:
    case YUV420:
        SetDest(ptr,
                ptr + m_lPitch[0] * m_Height,
                ptr + (m_lPitch[1]/2 + m_lPitch[0]) * m_Height);
        break;

    case YUV422:
    case YUV444:
        SetDest(ptr,
                ptr + m_lPitch[0] * m_Height,
                ptr + (m_lPitch[1] + m_lPitch[0]) * m_Height);
        break;

    case NV12:
        SetDest(ptr,
                ptr + m_lPitch[0] * m_Height,
                NULL);
        break;

    default:
        SetDest(ptr);
        break;
    }

    return UMC_OK;

} // Status VideoData_V51::SetPointer(vm_byte *ptr, size_t bytes)

} // end namespace UMC
