/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __VM_DEBUG_H__
#define __VM_DEBUG_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* ============================================================================
// Define ASSERT and VERIFY for debugging purposes
*/
#ifdef VM_DEBUG
#include <assert.h>
#define VM_ASSERT(f) assert((f))
#define VM_VERIFY(f) VM_ASSERT((f))
#else /* VM_DEBUG */
#ifdef  assert
#undef  assert
#endif
#define assert VM_ASSERT
#define VM_ASSERT(f) ((void) 0)
#define VM_VERIFY(f) ((void) (f))
#endif /* VM_DEBUG */

#if defined(_WIN32_WCE)
#if (_WIN32_WCE == 0x0300)
#define VM_ASSERT(exp) ((void)0)
#endif /* (_WIN32_WCE == 0x0300) */
#endif /* defined(_WIN32_WCE) */

/* ============================================================================
// CMC END
*/

void vm_message(const vm_char *format, ...);

/* VM_DEBUG can be used to selectively output debugging message.
 * Bit 0:           critical errors.
 * Bit 1:           warnings
 * Bit 2:           general infomation
 * Bit 3-31:        user defined information
 */

/* Functions to display a debug message */
void vm_debug_msg(int level, const vm_char *format, ...);
void vm_debug_trace(int level, const vm_char *format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_DEBUG_H__ */
