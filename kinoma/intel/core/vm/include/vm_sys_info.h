/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_SYS_INFO_H__
#define __VM_SYS_INFO_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Processor's feature bits */
enum
{
    CMOV_FEATURE                = 0x00001,
    MMX_FEATURE                 = 0x00002,
    MMX_EXT_FEATURE             = 0x00004,
    SSE_FEATURE                 = 0x00008,
    SSE2_FEATURE                = 0x00010,
    SSE3_FEATURE                = 0x00020,

    UNK_MANUFACTURE             = 0x00000,
    INTEL_MANUFACTURE           = 0x10000

};

/* Processors ID list */
enum
{
    UNKNOWN_CPU                 = 0,

    PENTIUM                     = INTEL_MANUFACTURE,
    PENTIUM_PRO                 = PENTIUM | CMOV_FEATURE,
    PENTIUM_MMX                 = PENTIUM | MMX_FEATURE,
    PENTIUM_2                   = PENTIUM_MMX | CMOV_FEATURE,
    PENTIUM_3                   = PENTIUM_2 | MMX_EXT_FEATURE | SSE_FEATURE,
    PENTIUM_4                   = PENTIUM_3 | SSE2_FEATURE,
    PENTIUM_4_PRESCOTT          = PENTIUM_4 | SSE3_FEATURE

};

typedef enum
{
    DDMMYY = 0,
    MMDDYY = 1,
    YYMMDD = 2

} DateFormat;

typedef enum
{
    HHMMSS      = 0,
    HHMM        = 1,
    HHMMSSMS1   = 2,
    HHMMSSMS2   = 3,
    HHMMSSMS3   = 4

} TimeFormat;

/* Function to obtain processor's specific information */
vm_var32 vm_sys_info_get_cpu_num(void);
void vm_sys_info_get_cpu_name(vm_char *cpu_name);
void vm_sys_info_get_date(vm_char *m_date, DateFormat df);
void vm_sys_info_get_time(vm_char *m_time, TimeFormat tf);
void vm_sys_info_get_vga_card(vm_char *vga_card);
void vm_sys_info_get_os_name(vm_char *os_name);
void vm_sys_info_get_computer_name(vm_char *computer_name);
void vm_sys_info_get_program_name(vm_char *program_name);
void vm_sys_info_get_program_path(vm_char *program_path);
void vm_sys_info_get_program_description(vm_char *program_description);
vm_var32 vm_sys_info_get_cpu_speed(void);
vm_var32 vm_sys_info_get_mem_size(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_SYS_INFO_H__ */
