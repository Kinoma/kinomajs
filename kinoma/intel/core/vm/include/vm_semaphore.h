/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_SEMAPHORE_H__
#define __VM_SEMAPHORE_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Invalidate a semaphore */
void vm_semaphore_set_invalid(vm_semaphore *sem);

/* Verify if a semaphore is valid */
int vm_semaphore_is_valid(vm_semaphore *sem);

/* Init a semaphore with value, return VM_OK if successful */
vm_status vm_semaphore_init(vm_semaphore *sem, int init_count);

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_timedwait(vm_semaphore *sem, unsigned int msec);

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_wait(vm_semaphore *sem);

/* Decrease the semaphore value without blocking, return VM_OK if success */
vm_status vm_semaphore_try_wait(vm_semaphore *sem);

/* Increase the semaphore value */
vm_status vm_semaphore_post(vm_semaphore *sem);

/* Destory a semaphore */
void vm_semaphore_destroy(vm_semaphore *sem);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_SEMAPHORE_H__ */
