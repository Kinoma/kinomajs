/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_MUTEX_H__
#define __VM_MUTEX_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Invalidate a mutex */
void vm_mutex_set_invalid(vm_mutex *mutex);

/* Verify if a mutex is valid */
int  vm_mutex_is_valid(vm_mutex *mutex);

/* Init a mutex, return VM_OK if success */
vm_status vm_mutex_init(vm_mutex *mutex);

/* Lock the mutex with blocking. */
vm_status vm_mutex_lock(vm_mutex *mutex);

/* Unlock the mutex. */
vm_status vm_mutex_unlock(vm_mutex *mutex);

/* Lock the mutex without blocking, return VM_OK if success */
vm_status vm_mutex_try_lock(vm_mutex *mutex);

/* Destory a mutex */
void vm_mutex_destroy(vm_mutex *mutex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_MUTEX_H__ */
