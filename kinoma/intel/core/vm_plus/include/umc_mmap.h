/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_MMAP_H
#define __UMC_MMAP_H

#include "vm_debug.h"
#include "vm_mmap.h"
#include "umc_structures.h"

namespace UMC
{

class MMap
{
public:
    // Default constructor
    MMap(void);
    // Destructor
    virtual ~MMap(void);

    // Initialize object
    Status Init(vm_char *sz_file);
    // Map memory
    Status Map(vm_sizet st_offset, vm_sizet st_sizet);
    // Get addres of mapping
    void *GetAddr(void);
    // Get offset of mapping
    vm_sizet GetOffset(void);
    // Get size of mapping
    vm_sizet GetSize(void);
    // Get size of mapped file
    vm_sizet GetFileSize(void);

protected:
    vm_mmap m_handle;                                           // (vm_mmap) handle to system mmap object
    void *m_address;                                            // (void *) addres of mapped window
    vm_sizet m_file_size;                                       // (vm_sizet) file size
    vm_sizet m_offset;                                          // (vm_sizet) offset of mapping
    vm_sizet m_sizet;                                           // (vm_sizet) size of window
};

inline
void *MMap::GetAddr(void)
{
    return m_address;

} // void *MMap::GetAddr(void)

inline
vm_sizet MMap::GetOffset(void)
{
    return m_offset;

} // vm_sizet MMap::GetOffset(void)

inline
vm_sizet MMap::GetSize(void)
{
    return m_sizet;

} // vm_sizet MMap::GetSize(void)

inline
vm_sizet MMap::GetFileSize(void)
{
    return m_file_size;

} // vm_sizet MMap::GetFileSize(void)

} // namespace UMC

#endif // __UMC_MMAP_H
