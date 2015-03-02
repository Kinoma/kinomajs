/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_MUTEX_H
#define __UMC_MUTEX_H

#include "vm_debug.h"
#include "vm_mutex.h"
#include "umc_structures.h"

namespace UMC
{

class Mutex
{
public:
    // Default constructor
    Mutex(void);
    // Destructor
    virtual ~Mutex(void);

    // Initialize mutex
    Status Init(void);
    // Get ownership of mutex
    void Lock(void);
    // Release ownership of mutex
    void Unlock(void);
    // Try to get ownership of mutex
    Status TryLock(void);
    // Get ownership of initialized mutex
    void LockIfInitialized(void);
    // Release ownership of initialized mutex
    void UnlockIfInitialized(void);
    // Is mutex inited
    bool IsInited(void);

protected:
    vm_mutex m_handle;                                          // (vm_mutex) handle to system mutex

};

inline
void Mutex::Lock(void)
{
    Status umcRes = UMC_OK;

    if (!vm_mutex_is_valid(&m_handle))
        umcRes = Init();

    if ((UMC_OK == umcRes) &&
        (VM_OK != vm_mutex_lock(&m_handle)))
        assert(false);

} // void Mutex::Lock(void)

inline
void Mutex::Unlock(void)
{
    Status umcRes = UMC_OK;

    if (!vm_mutex_is_valid(&m_handle))
        umcRes = Init();
    else if (VM_OK != vm_mutex_unlock(&m_handle))
        assert(false);

} // void Mutex::Unlock(void)

inline
Status Mutex::TryLock(void)
{
    Status umcRes = UMC_OK;

    if (!vm_mutex_is_valid(&m_handle))
        umcRes = Init();

    if (UMC_OK == umcRes)
        umcRes = vm_mutex_try_lock(&m_handle);

    return umcRes;

} // Status Mutex::TryLock(void)

inline
void Mutex::LockIfInitialized(void)
{
    if (vm_mutex_is_valid(&m_handle) &&
        (VM_OK != vm_mutex_lock(&m_handle)))
        assert(false);

} // void Mutex::LockIfInitialized(void)

inline
void Mutex::UnlockIfInitialized(void)
{
    if (vm_mutex_is_valid(&m_handle) &&
        (VM_OK != vm_mutex_unlock(&m_handle)))
        assert(false);

} // void Mutex::UnlockIfInitialized(void)

inline
bool Mutex::IsInited(void)
{
    return (0 != vm_mutex_is_valid(&m_handle));

} // bool Mutex::IsInited(void)

} // namespace UMC

#endif // __UMC_MUTEX_H
