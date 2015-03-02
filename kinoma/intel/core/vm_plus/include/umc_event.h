/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_EVENT_H
#define __UMC_EVENT_H

#include "vm_debug.h"
#include "vm_event.h"
#include "umc_structures.h"

namespace UMC
{

class Event
{
public:
    // Default constructor
    Event(void);
    // Destructor
    virtual ~Event(void);

    // Initialize event
    Status Init(int iManual, int iState);
    // Set event to signaled state
    Status Set(void);
    // Set event to non-signaled state
    Status Reset(void);
    // Pulse event (should not be used)
    Status Pulse(void);
    // Wait until event is signaled
    Status Wait(unsigned int msec);
    // Infinitely wait until event is signaled
    Status Wait(void);

protected:
    vm_event m_handle;                                          // (vm_event) handle to system event
};

inline
Status Event::Set(void)
{
    return vm_event_signal(&m_handle);

} // Status Event::Set(void)

inline
Status Event::Reset(void)
{
    return vm_event_reset(&m_handle);

} // Status Event::Reset(void)

inline
Status Event::Pulse(void)
{
    return vm_event_pulse(&m_handle);

} // Status Event::Pulse(void)

inline
Status Event::Wait(unsigned int msec)
{
    return vm_event_timed_wait(&m_handle, msec);

} // Status Event::Wait(unsigned int msec)

inline
Status Event::Wait(void)
{
    return vm_event_wait(&m_handle);

} // Status Event::Wait(void)

} // namespace UMC

#endif // __UMC_EVENT_H
