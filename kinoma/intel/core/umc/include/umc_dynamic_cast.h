/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_DYNAMIC_CAST_H
#define __UMC_DYNAMIC_CAST_H

#include <stdlib.h> //  for size_t under Linux
#include <string.h>
#include "vm_types.h"
#include "vm_strings.h"

//  There are functional for dynamic cast (linear time casting).
//  Multiple inheritance is no acceptable.
//  Weak conversion is used to casting objects,
//  which created in different places & heaps (ex. EXE & DLL).

//  Declare "dynamic cast" type
typedef const char *(*pDynamicCastFunction)(void);

//  This define must be placed in first line of base class declaration (.H file)
#define DYNAMIC_CAST_DECL_BASE(class_name) \
    public: \
    /* declare function to obtain string with class name */ \
    static const char *GetClassName(void) {return #class_name;} \
    /* strong casting - compare function adresses of classes */ \
    virtual bool TryStrongCasting(pDynamicCastFunction pCandidateFunction) const \
    { \
        return (pCandidateFunction == &class_name::GetClassName); \
    } \
    /* weak casting - compare names of classes */ \
    virtual bool TryWeakCasting(pDynamicCastFunction pCandidateFunction) const \
    { \
        return (0 == strcmp(#class_name, pCandidateFunction())); \
    }

//  This define must be placed in first line of descendant class declaration (.H file)
#define DYNAMIC_CAST_DECL(class_name, parent_class) \
    public: \
    /* declare function to obtain string with class name */ \
    static const char *GetClassName(void) {return #class_name;} \
    /* strong casting - compare function adresses of classes */ \
    virtual bool TryStrongCasting(pDynamicCastFunction pCandidateFunction) const \
    { \
        if (pCandidateFunction == &class_name::GetClassName) \
            return true; \
        return parent_class::TryStrongCasting(pCandidateFunction); \
    } \
    /* weak casting - compare names of classes */ \
    virtual bool TryWeakCasting(pDynamicCastFunction pCandidateFunction) const \
    { \
        if (0 == strcmp(#class_name, pCandidateFunction())) \
            return true; \
        return parent_class::TryWeakCasting(pCandidateFunction); \
    }

// Function for dynamic cast from one class to another one (forward declaration)
template <class To, class From> To *DynamicCast(From *pFrom);
// Function for dynamic cast from one class to another one (inline implementation)
template <class To, class From> To *DynamicCast(From *pFrom)
{
    // some compiler complain to compare pointer and zero.
    // we use real zero pointer instead const zero.
    void *NullPointer = 0;

    // we don't need do casting
    if (NullPointer == pFrom)
        return reinterpret_cast<To *> (NullPointer);

    // try strong casting
    if (pFrom->TryStrongCasting(&To::GetClassName))
        return reinterpret_cast<To *> (pFrom);

    // there are no strong conversion, try weak casting
    if (pFrom->TryWeakCasting(&To::GetClassName))
        return reinterpret_cast<To *> (pFrom);

    // there are no any conversion
    return reinterpret_cast<To *> (NullPointer);

} // template <class To, class From> lpTo *DynamicCast(From *pFrom)

#endif /* __UMC_DYNAMIC_CAST_H */
