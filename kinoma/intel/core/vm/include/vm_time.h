/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_TIME_H__
#define __VM_TIME_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef vm_var64 vm_tick;

/* Yield the execution of current thread for msec miliseconds */
void vm_time_sleep(unsigned int msec);

/* Obtain the clock tick of an uninterrupted master clock */
vm_tick vm_time_get_tick(void);

/* Obtain the clock resolution */
vm_tick vm_time_get_frequency(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_TIME_H__ */
