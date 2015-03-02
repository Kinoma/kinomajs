/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "umc_media_data.h"

namespace UMC
{

MediaData_V51::MediaData_V51(size_t length)
{
    m_pBufferPointer   = NULL;
    m_pDataPointer     = NULL;
    m_nBufferSize      = 0;
    m_nDataSize        = 0;
    m_pts_start        = -1;
    m_pts_end          = 0;

    m_bMemoryAllocated = 0;

    if (length)
    {
        m_pBufferPointer = new vm_byte[length];
        if (m_pBufferPointer)
        {
            m_pDataPointer     = m_pBufferPointer;
            m_nBufferSize      = length;
            m_bMemoryAllocated = 1;
        }
    }
	m_refCon[0] = m_refCon[1] = 0;

} // MediaData_V51::MediaData_V51(size_t length) :

MediaData_V51::~MediaData_V51()
{
    Close();

} // MediaData_V51::~MediaData_V51()

Status MediaData_V51::Close(void)
{
    if (m_pBufferPointer && m_bMemoryAllocated)
        delete[] m_pBufferPointer;

    m_pBufferPointer   = NULL;
    m_pDataPointer     = NULL;
    m_nBufferSize      = 0;
    m_nDataSize        = 0;

    m_bMemoryAllocated = 0;

    return UMC_OK;

} // Status MediaData_V51::Close(void)

Status MediaData_V51::SetDataSize(size_t bytes)
{
    if (!m_pBufferPointer)
        return UMC_NULL_PTR;

    if (bytes > (m_nBufferSize - (m_pDataPointer - m_pBufferPointer)))
        return UMC_OPERATION_FAILED;

    m_nDataSize = bytes;

    return UMC_OK;

} // Status MediaData_V51::SetDataSize(size_t bytes)

// Set the pointer to a buffer allocated by the user
// bytes define the size of buffer
// size of data is equal to buffer size after this call
Status MediaData_V51::SetBufferPointer(vm_byte *ptr, size_t size)
{
    // release object
    Close();

    // set new value(s)
    m_pBufferPointer  = ptr;
    m_pDataPointer    = ptr;
    m_nBufferSize     = size;
    m_nDataSize       = size;

    return UMC_OK;

} // Status MediaData_V51::SetBufferPointer(vm_byte *ptr, size_t size)

Status MediaData_V51::SetTime(double start, double end)
{
 //   if (start < 0  && start != -1.0)
 //       return UMC_OPERATION_FAILED;

    m_pts_start = start;
    m_pts_end = end;

    return UMC_OK;

} // Status MediaData_V51::SetTime(double start, double end)

Status MediaData_V51::GetTime(double& start, double& end)
{
    start = m_pts_start;
    end = m_pts_end;

    return UMC_OK;

} // Status MediaData_V51::GetTime(double& start, double& end)

Status MediaData_V51::MoveDataPointer(int bytes)
{
    if (bytes >= 0 && m_nDataSize >= (size_t)bytes) {
        m_pDataPointer   += bytes;
        m_nDataSize      -= bytes;

        return UMC_OK;
    } else if (bytes < 0 && (size_t)(m_pDataPointer - m_pBufferPointer) >= (size_t)(-bytes)) {
        m_pDataPointer   += bytes;
        m_nDataSize      -= bytes;

        return UMC_OK;
    }

    return UMC_OPERATION_FAILED;
} // Status MediaData_V51::MovePointer(int bytes)

MediaData_V51& MediaData_V51::operator = (MediaData_V51& src)
{
    Close();

    m_pts_start        = src.m_pts_start;
    m_pts_end          = src.m_pts_end;
    m_nBufferSize      = src.m_nBufferSize;
    m_nDataSize        = src.m_nDataSize;

    m_pDataPointer     = src.m_pDataPointer;
    m_pBufferPointer   = src.m_pBufferPointer;
    m_bMemoryAllocated = false;

    return *this;
} // MediaData_V51& MediaData_V51::operator = (MediaData_V51& src)

} // namespace UMC
