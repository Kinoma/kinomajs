/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#if !defined(__SYMBIAN32__) && (defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE))

#include <windows.h>

#if defined(_WIN32_WCE)
#include <winsock.h>
#endif /* defined(_WIN32_WCE) */

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
    HANDLE handle;

} vm_event;

/* vm_mmap.h */
typedef struct vm_mmap
{
    HANDLE fd_file, fd_map;
    void  *address;
    int   fAccessAttr;

} vm_mmap;

/* vm_mutex.h */
typedef struct vm_mutex
{
    CRITICAL_SECTION sCritSection;
    int iInited;

} vm_mutex;

/* vm_semaphore.h */
typedef struct vm_semaphore
{
    HANDLE handle;

} vm_semaphore;

/* vm_thread.h */
typedef struct vm_thread
{
    HANDLE handle;
    vm_mutex access_mut;
    int i_wait_count;

} vm_thread;

/* vm_socket.h */
#define VM_SOCKET_QUEUE 20
typedef struct vm_socket
{
    fd_set r_set, w_set;
    SOCKET chns[VM_SOCKET_QUEUE + 1];
    struct sockaddr_in sal;
    struct sockaddr_in sar;
    struct sockaddr_in peers[VM_SOCKET_QUEUE + 1];
    int flags;

} vm_socket;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE) */
