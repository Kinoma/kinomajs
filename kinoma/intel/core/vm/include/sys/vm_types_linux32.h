/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifdef LINUX32

#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>

#ifdef __INTEL_COMPILER
/* ICC and Fedora Core 3 incompatibility */
#define __interface xxinterface
#include <netinet/in.h>
#undef __interface
#else /* __INTEL_COMPILER */
#include <netinet/in.h>
#endif /* __INTEL_COMPILER */

#include <sys/socket.h>
#include <sys/select.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef unsigned char vm_var8;
typedef unsigned short int vm_var16;
typedef unsigned int vm_var32;
typedef int vm_var32s;
typedef unsigned long long vm_var64;
typedef unsigned long long vm_var64u;
typedef signed long long vm_var64s;
typedef unsigned char vm_byte;

typedef unsigned long vm_sizet;

#define VM_ALIGN_DECL(X,Y) __attribute__ ((aligned(X))) Y

#define CONST_LL(X) X##LL
#define CONST_ULL(X) X##ULL

/* vm_event.h */
typedef struct vm_event
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int manual;
    int state;

} vm_event;

/* vm_mmap.h */
typedef struct vm_mmap
{
    int fd;
    void *address;
    size_t sizet;
    int fAccessAttr;

} vm_mmap;

/* vm_mutex.h */
typedef struct vm_mutex
{
    pthread_mutex_t handle;
    int is_valid;

} vm_mutex;

/* vm_semaphore.h */
typedef struct vm_semaphore
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int count;

} vm_semaphore;

/* vm_thread.h */
typedef struct vm_thread
{
    pthread_t handle;
    int is_valid;
    unsigned int (*p_thread_func)(void *);
    void *p_arg;
    vm_event exit_event;
    vm_mutex access_mut;
    int i_wait_count;

} vm_thread;

/* vm_socket.h */
#define VM_SOCKET_QUEUE 20
typedef struct vm_socket
{
   fd_set r_set, w_set;
   int chns[VM_SOCKET_QUEUE+1];
   struct sockaddr_in sar;
   struct sockaddr_in peers[VM_SOCKET_QUEUE+1];
   int flags;

} vm_socket;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINUX32 */
