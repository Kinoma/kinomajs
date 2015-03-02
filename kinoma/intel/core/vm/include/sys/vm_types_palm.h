/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2005 Intel Corporation. All Rights Reserved.
//
*/

#if defined(_PALM) || defined(__SYMBIAN32__)


#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef unsigned char vm_var8;
typedef unsigned short int vm_var16;
typedef unsigned int vm_var32;
typedef int vm_var32s;
typedef unsigned __int64 vm_var64;
typedef unsigned __int64 vm_var64u;
typedef signed __int64 vm_var64s;
typedef unsigned char vm_byte;

typedef unsigned __int64 vm_sizet;

typedef char vm_char;
#define VM_STRING(x) x

#ifdef __ICL

#define VM_ALIGN_DECL(X,Y) __declspec(align(X)) Y

#else /* !__ICL */

#define VM_ALIGN_DECL(X,Y) Y

#endif /* __ICL */

#define CONST_LL(X) X
#define CONST_ULL(X) X##ULL


/* vm_event.h */
typedef struct vm_event
{
   void *state;
} vm_event;

/* vm_mmap.h */
typedef struct vm_mmap
{
   void *state;
} vm_mmap;

/* vm_mutex.h */
typedef struct vm_mutex
{
   void *state;
} vm_mutex;

/* vm_semaphore.h */
typedef struct vm_semaphore
{
   void *state;
} vm_semaphore;

/* vm_thread.h */
typedef struct vm_thread
{
   void *state;
} vm_thread;

/* vm_socket.h */
#define VM_SOCKET_QUEUE 20
typedef struct vm_socket
{
   void *state;
} vm_socket;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE) */
