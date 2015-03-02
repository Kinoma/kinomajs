/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//    Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/

#ifndef __UMC_AUTOMATIC_MUTEX_H
#define __UMC_AUTOMATIC_MUTEX_H

#include "vm_mutex.h"

namespace UMC
{

#pragma pack(1)

class AutomaticMutex
{
public:
    // Constructor
    AutomaticMutex(vm_mutex &mutex)
    {
        if (vm_mutex_is_valid(&mutex))
        {
            m_pMutex = &mutex;

            // lock mutex
            vm_mutex_lock(m_pMutex);
            m_bLocked = true;
        }
        else
        {
            m_pMutex = NULL;
            m_bLocked = false;
        }
    }

    // Destructor
    ~AutomaticMutex(void)
    {
        Unlock();
    }

    // lock mutex again
    void Lock(void)
    {
        if (m_pMutex)
        {
            if ((vm_mutex_is_valid(m_pMutex)) && (false == m_bLocked))
            {
                vm_mutex_lock(m_pMutex);
                m_bLocked = true;
            }
        }
    }

    // Unlock mutex
    void Unlock(void)
    {
        if (m_pMutex)
        {
            if ((vm_mutex_is_valid(m_pMutex)) && (m_bLocked))
            {
                vm_mutex_unlock(m_pMutex);
                m_bLocked = false;
            }

        }
    }

protected:

    vm_mutex *m_pMutex;                                         // (vm_mutex *) pointer to using mutex
    bool m_bLocked;                                             // (bool) mutex is own locked

};

#pragma pack()

} // end namespace UMC

#endif // __UMC_AUTOMATIC_MUTEX_H
