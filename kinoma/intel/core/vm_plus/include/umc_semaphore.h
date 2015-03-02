/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_SEMAPHORE_H
#define __UMC_SEMAPHORE_H

#include "vm_debug.h"
#include "vm_semaphore.h"
#include "umc_structures.h"

namespace UMC
{

class Semaphore
{
public:
    // Default constructor
    Semaphore(void);
    // Destructor
    virtual ~Semaphore(void);

    // Initialize semaphore
    Status Init(int iInitCount);
    // Check semaphore state
    bool IsValid(void);
    // Try to obtain semaphore
    Status TryWait(void);
    // Wait until semaphore is signaled
    Status Wait(unsigned int msec);
    // Infinitely wait until semaphore is signaled
    Status Wait(void);
    // Set semaphore to signaled state
    Status Signal(void);

protected:
    vm_semaphore m_handle;                                      // (vm_semaphore) handle to system semaphore
};

inline
bool Semaphore::IsValid(void)
{
    return vm_semaphore_is_valid(&m_handle) ? true : false;

} // bool Semaphore::IsValid(void)

inline
Status Semaphore::TryWait(void)
{
    return vm_semaphore_try_wait(&m_handle);

} // Status Semaphore::TryWait(void)

inline
Status Semaphore::Wait(unsigned int msec)
{
    return vm_semaphore_timedwait(&m_handle, msec);

} // Status Semaphore::Wait(unsigned int msec)

inline
Status Semaphore::Wait(void)
{
    return vm_semaphore_wait(&m_handle);

} // Status Semaphore::Wait(void)

inline
Status Semaphore::Signal(void)
{
    return vm_semaphore_post(&m_handle);

} // Status Semaphore::Signal(void)

} // namespace UMC

#endif // __UMC_SEMAPHORE_H
