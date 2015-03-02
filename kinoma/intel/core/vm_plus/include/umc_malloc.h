/* /////////////////////////////////////////////////////////////////////////////// */
/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
//
*/

#ifndef __UMC_MALLOC_H
#define __UMC_MALLOC_H

//#define VM_MALLOC             // define this macro to use "vm memory allocation" instead of regular
//#define VM_MALLOC_STATISTIC   // define this macro to turn on memory tracing into the c:/malloc.csv file

#if defined(VM_MALLOC)
    #include <stdlib.h>     // declare native malloc functions to avoid redefinition it by vm_malloc
    #include <malloc.h>     // declare native malloc functions to avoid redefinition it by vm_malloc
    #include "ipps.h"
    #include "vm_types.h"

    class VM_MallocItem;
    class VM_MallocItemArray
    {
        VM_MallocItem** m_array;
        vm_var32        m_count;
        vm_var32        m_allocated;
        vm_var32s       m_mem_usage_max;
        vm_var32s       m_mem_usage_current;

    public:

         VM_MallocItemArray();
        ~VM_MallocItemArray();

        void     AddItem(VM_MallocItem* item);
        void     DeleteItem(void* lpv);

        vm_var32s GetMemUsageMax()     { return m_mem_usage_max;     }
        vm_var32s GetMemUsageCurrent() { return m_mem_usage_current; }

        void     ChangeMemUsage(vm_var32s size);
    };

    extern VM_MallocItemArray vm_malloc_array;

    #define     vm_args_malloc   const vm_char* lpcFileName = 0, vm_var32 iStringNumber = 0, const vm_char* lpcClassName = 0

    void*       vm_malloc         (            vm_var32s size, vm_args_malloc);
    void*       vm_calloc         (size_t num, vm_var32s size, vm_args_malloc);
    void*       vm_realloc        (void *lpv,  vm_var32s size, vm_args_malloc);
    void        vm_free           (void *lpv);

    Ipp8u*      vm_ippsMalloc_8u  (            vm_var32s size, vm_args_malloc);
    Ipp16u*     vm_ippsMalloc_16u (            vm_var32s size, vm_args_malloc);
    Ipp32u*     vm_ippsMalloc_32u (            vm_var32s size, vm_args_malloc);
    Ipp8s*      vm_ippsMalloc_8s  (            vm_var32s size, vm_args_malloc);
    Ipp16s*     vm_ippsMalloc_16s (            vm_var32s size, vm_args_malloc);
    Ipp32s*     vm_ippsMalloc_32s (            vm_var32s size, vm_args_malloc);
    Ipp64s*     vm_ippsMalloc_64s (            vm_var32s size, vm_args_malloc);
    Ipp32f*     vm_ippsMalloc_32f (            vm_var32s size, vm_args_malloc);
    Ipp64f*     vm_ippsMalloc_64f (            vm_var32s size, vm_args_malloc);
    Ipp8sc*     vm_ippsMalloc_8sc (            vm_var32s size, vm_args_malloc);
    Ipp16sc*    vm_ippsMalloc_16sc(            vm_var32s size, vm_args_malloc);
    Ipp32sc*    vm_ippsMalloc_32sc(            vm_var32s size, vm_args_malloc);
    Ipp64sc*    vm_ippsMalloc_64sc(            vm_var32s size, vm_args_malloc);
    Ipp32fc*    vm_ippsMalloc_32fc(            vm_var32s size, vm_args_malloc);
    Ipp64fc*    vm_ippsMalloc_64fc(            vm_var32s size, vm_args_malloc);
    void        vm_ippsFree       (void *lpv);

    void* operator new            (size_t size);
    void* operator new            (size_t size, const vm_char* lpcFileName, vm_var32 iStringNumber, const vm_char* lpcClassName);
    void  operator delete         (void *lpv);
    void  operator delete         (void *lpv,   const vm_char* lpcFileName, vm_var32 iStringNumber, const vm_char* lpcClassName);

    void vm_malloc_measure(vm_var32 start, vm_args_malloc);

    #if !defined(VM_MALLOC_OWN)
        #if defined(VM_MALLOC_STATISTIC)
            #if defined(VM_MALLOC_GLOBAL)
                #define vm_args_info    __FILE__, __LINE__, 0
            #else
                #define vm_args_info    __FILE__, __LINE__, GetClassName()
            #endif
        #else
                #define vm_args_info    0, 0, 0
        #endif

        #define new                    new         (           vm_args_info)
        #define malloc(         size) vm_malloc    (     size, vm_args_info)
        #define calloc(    num, size) vm_calloc    (num, size, vm_args_info)
        #define realloc(   lpv, size) vm_realloc   (lpv, size, vm_args_info)

        #define ippsMalloc_8u(  size) vm_ippsMalloc_8u  (size, vm_args_info)
        #define ippsMalloc_16u( size) vm_ippsMalloc_16u (size, vm_args_info)
        #define ippsMalloc_32u( size) vm_ippsMalloc_32u (size, vm_args_info)
        #define ippsMalloc_8s(  size) vm_ippsMalloc_8s  (size, vm_args_info)
        #define ippsMalloc_16s( size) vm_ippsMalloc_16s (size, vm_args_info)
        #define ippsMalloc_32s( size) vm_ippsMalloc_32s (size, vm_args_info)
        #define ippsMalloc_64s( size) vm_ippsMalloc_64s (size, vm_args_info)
        #define ippsMalloc_32f( size) vm_ippsMalloc_32f (size, vm_args_info)
        #define ippsMalloc_64f( size) vm_ippsMalloc_64f (size, vm_args_info)
        #define ippsMalloc_8sc( size) vm_ippsMalloc_8sc (size, vm_args_info)
        #define ippsMalloc_16sc(size) vm_ippsMalloc_16sc(size, vm_args_info)
        #define ippsMalloc_32sc(size) vm_ippsMalloc_32sc(size, vm_args_info)
        #define ippsMalloc_64sc(size) vm_ippsMalloc_64sc(size, vm_args_info)
        #define ippsMalloc_32fc(size) vm_ippsMalloc_32fc(size, vm_args_info)
        #define ippsMalloc_64fc(size) vm_ippsMalloc_64fc(size, vm_args_info)

        #define free(lpv)     vm_free    (lpv)
        #define ippsFree(lpv) vm_ippsFree(lpv)

        #define vm_malloc_start_measure     vm_malloc_measure(1, vm_args_info);
        #define vm_malloc_finish_measure    vm_malloc_measure(0, vm_args_info);
    #endif //VM_MALLOC_OWN
#else //VM_MALLOC
        #define vm_malloc_start_measure
        #define vm_malloc_finish_measure
#endif //VM_MALLOC

#endif //__UMC_MALLOC_H
