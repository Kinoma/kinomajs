/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_TRACE_LOG_H__
#define __VM_TRACE_LOG_H__

#include "vm_debug.h"
#include "vm_types.h"

#if defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)

#define _FILE_ _tcsrchr(__FILE__, __VM('\\')) + 1
#define _LINE_ __LINE__
#define _CLASS_NAME_ _tcsrchr(typeid(*this).name(), __VM(' ')) + 1

#else /* !(defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)) */

#define _FILE_
#define _LINE_

#endif /* defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE) */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef enum
{
    NEVER = 0,
    WITHDATE,
    WITHTIME,
    WITHMEM,
    WITHALL

} vm_trace_atribute;

/* Function to making log */
void vm_trace_log(vm_trace_atribute atrib, vm_char *output);
void *vm_open_logfile(vm_char *log_filename, vm_char *file_ini, vm_char *dir);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_TRACE_LOG_H__ */
