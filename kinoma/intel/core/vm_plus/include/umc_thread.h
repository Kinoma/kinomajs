/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_THREAD_H
#define __UMC_THREAD_H

#include "vm_debug.h"
#include "vm_thread.h"
#include "umc_structures.h"

namespace UMC
{
class Thread
{
public:
    // Default constructor
    Thread(void);
    virtual ~Thread(void);

    // Check thread status
    bool IsValid(void);
    // Create new thread
    Status Create(vm_thread_callback func, void *arg);
    // Wait until thread does exit
    void Wait(void);
    // Set thread priority
    Status SetPriority(vm_thread_priority priority);
    // Close thread object
    void Close(void);

protected:
    vm_thread m_Thread;                                         // (vm_thread) handle to system thread

};

inline
bool Thread::IsValid(void)
{
    return vm_thread_is_valid(&m_Thread) ? true : false;

} // bool Thread::IsValid(void)

inline
void Thread::Wait(void)
{
    vm_thread_wait(&m_Thread);

} // void Thread::Wait(void)

inline
void Thread::Close(void)
{
    vm_thread_close(&m_Thread);

} // void Thread::Close(void)

} // namespace UMC

#endif // __UMC_THREAD_H
