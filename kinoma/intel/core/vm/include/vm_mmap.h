/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_MMAP_H__
#define __VM_MMAP_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* flags */
enum
{
    FLAG_ATTRIBUTE_READ         = 0x00000001,
    FLAG_ATTRIBUTE_WRITE        = 0x00000002

};


/* Set the mmap handle to be invalid */
void vm_mmap_set_invalid(vm_mmap *handle);

/* Verify if the mmap handle is valid */
int  vm_mmap_is_valid(vm_mmap *handle);

/* Map a file into system meory, return size of the mapped file */
vm_sizet vm_mmap_create(vm_mmap *handle, vm_char *file, int fileAttr);

/* Obtain a view of the mapped file, return the page aligned offset & size */
void *vm_mmap_set_view(vm_mmap *handle, vm_sizet *offset, vm_sizet *size);

/* Delete the mmap handle */
void vm_mmap_close(vm_mmap *handle);

/*  Return page size*/
vm_var32 vm_mmap_get_page_size(void);

/*  Return allocation granularity*/
vm_var32 vm_mmap_get_alloc_granularity(void);

/* Unmap the mmap handle */
void vm_mmap_unmap(vm_mmap *handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_MMAP_H__ */
