/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_SHARED_OBJECT_H
#define __VM_SHARED_OBJECT_H

#include "vm_types.h"
#include "vm_strings.h"


typedef void* vm_so_handle;

#ifdef __cplusplus
extern "C" {
#endif    //    __cplusplus

vm_so_handle vm_so_load(vm_char* so_file_name);
void*        vm_so_get_addr(vm_so_handle so_handle,vm_char* so_func_name);
void         vm_so_free(vm_so_handle so_handle);

#ifdef __cplusplus
}
#endif    //    __cplusplus


#ifndef NULL
#define NULL (void*)0L
#endif

#endif//__VM_SHARED_OBJECT_H
