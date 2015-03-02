/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//          Intel(R) Integrated Performance Primitives
//               Core (ippcore)
//
*/

#if !defined( __IPPCORE_H__ ) || defined( _OWN_BLDPCS )
#define __IPPCORE_H__

#ifndef __IPPDEFS_H__
  #include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (_WIN32_WCE) && defined (_M_IX86) && defined (__stdcall)
  #define _IPP_STDCALL_CDECL
  #undef __stdcall
#endif

/* /////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                   Functions declarations
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////// */


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippGetLibVersion
//  Purpose:    getting of the library version
//  Returns:    the structure of information about version
//              of ippcore library
//  Parameters:
//
//  Notes:      not necessary to release the returned structure
*/
IPPAPI( const IppLibraryVersion*, ippGetLibVersion, (void) )


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippGetStatusString
//  Purpose:    convert the library status code to a readable string
//  Parameters:
//    StsCode   IPP status code
//  Returns:    pointer to string describing the library status code
//
//  Notes:      don't free the pointer
*/
IPPAPI( const char*, ippGetStatusString, ( IppStatus StsCode ) )


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippGetCpuType
//  Purpose:    detects Intel(R) processor
//  Parameter:  none
//  Return:     IppCpuType
//
*/

IPPAPI( IppCpuType, ippGetCpuType, (void) )

/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippGetCpuClocks
//  Purpose:    reading of time stamp counter (TSC) register value
//  Returns:    TSC value
//
//  Note:      An hardware exception is possible if TSC reading is not supported by
/              the current chipset
*/

IPPAPI( Ipp64u, ippGetCpuClocks, (void) )


/* ///////////////////////////////////////////////////////////////////////////
//  Names:  ippSetFlushToZero,
//          ippSetDenormAreZero.
//
//  Purpose: ippSetFlushToZero enables or disables the flush-to-zero mode,
//           ippSetDenormAreZero enables or disables the denormals-are-zeros
//           mode.
//
//  Arguments:
//     value       - !0 or 0 - set or clear the corresponding bit of MXCSR
//     pUMask      - pointer to user store current underflow exception mask
//                   ( may be NULL if don't want to store )
//
//  Return:
//   ippStsNoErr              - Ok
//   ippStsCpuNotSupportedErr - the mode is not suppoted
*/

IPPAPI( IppStatus, ippSetFlushToZero, ( int value, unsigned int* pUMask ))
IPPAPI( IppStatus, ippSetDenormAreZeros, ( int value ))



/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippAlignPtr
//  Purpose:    pointer aligning
//  Returns:    aligned pointer
//
//  Parameter:
//    ptr        - pointer
//    alignBytes - number of bytes to align
//
*/
IPPAPI( void*, ippAlignPtr, ( void * ptr, int alignBytes ) )

/* /////////////////////////////////////////////////////////////////////////////
//                   Functions to allocate and free memory
///////////////////////////////////////////////////////////////////////////// */
/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippMalloc
//  Purpose:    32-byte aligned memory allocation
//  Parameter:
//    len       number of bytes
//  Returns:    pointer to allocated memory
//
//  Notes:      the memory allocated by ippMalloc has to be free by ippFree
//              function only.
*/

IPPAPI( void*, ippMalloc,  (int length) )


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippFree
//  Purpose:    free memory allocated by the ippMalloc function
//  Parameter:
//    ptr       pointer to the memory allocated by the ippMalloc function
//
//  Notes:      use the function to free memory allocated by ippMalloc
*/
IPPAPI( void, ippFree, (void* ptr) )


/* /////////////////////////////////////////////////////////////////////////////
//                   Functions to control emerged library
///////////////////////////////////////////////////////////////////////////// */
/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippStaticInit
//  Purpose:    Automatic switching to to best for current cpu library code using.
//  Returns:
//   ippStsNoErr       - the best code (static) successfully set
//   ippStsNonIntelCpu - px version (static) of code was set
//   ippStsNoOperationInDll - function does nothing in the dynamic version of the library
//
//  Parameter:  nothing
//
//  Notes:      At the moment of this function excecution no any other IPP function
//              has to be working
*/
IPPAPI( IppStatus, ippStaticInit, ( void ))



/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippStaticInitCpu
//  Purpose:    switching to user defined target cpu library code using
//
//  Returns:
//   ippStsNoErr       - required target cpu library code is successfully set
//   ippStsCpuMismatch - required target cpu library can't be set, the previous
//                       set is used
//   ippStsNoOperationInDll - function does nothing in the dynamic version of the library
//
//  Parameter:  IppCpuType
//
//  Notes:      At the moment of this function excecution no any other IPP function
//              has to be working
*/
IPPAPI( IppStatus, ippStaticInitCpu, ( IppCpuType cpu ) )


/* ////////////////////////////////////////////////////////////////////////////
//  Name:       ippGetCpuFreqMhz
//
//  Purpose:    the function estimates cpu frequency and returns
//              its value in MHz as a integer
//
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         null pointer to the freq value
//    ippStsSizeErr            wrong num of tries, internal var
//  Arguments:
//    pMhz                     pointer to the integer to write
//                             cpu freq value estimated
//
//  Notes:      no exact value is guarantied, the value could
//              vary with cpu workloading
*/

IPPAPI(IppStatus, ippGetCpuFreqMhz, ( int* pMhz ) )

/* ////////////////////////////////////////////////////////////////////////////
//  Name:       ippSetNumThreads
//
//  Purpose:
//
//  Return:
//    ippStsNoErr              Ok
//    ippStsNoOperation        For static library internal threading is not suppoted
//    ippStsSizeErr            Desired number of threads less or equal zero
//
//  Arguments:
//    numThr                   Desired number of threads
*/
IPPAPI( IppStatus, ippSetNumThreads, ( int numThr ) )

/* ////////////////////////////////////////////////////////////////////////////
//  Name:       ippGetNumThreads
//
//  Purpose:
//
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         Pointer to numThr is Null
//    ippStsNoOperation        For static library internal threading is not suppoted
//                             and return value is always == 1
//
//  Arguments:
//    pNumThr                  Pointer to memory location where to store current numThr
*/
IPPAPI( IppStatus, ippGetNumThreads, (int* pNumThr) )

/* ////////////////////////////////////////////////////////////////////////////
//  Name:       ippGetMaxCacheSizeB
//
//  Purpose:  Definition maximal from the sizes of L2 or L3 in bytes
//
//  Return:
//    ippStsNullPtrErr         The result's pointer is NULL.
//    ippStsNotSupportedCpu    The cpu is not supported.
//    ippStsUnknownCacheSize   The cpu is supported, but the size of the cache is unknown.
//    ippStsNoErr              Ok
//
//  Arguments:
//    pSizeByte                Pointer to the result
//
//  Note:
//    1). Intel(R) processors are supported only.
//    2). Intel(R) Itanium(R) processors and platforms with Intel XScale(R) technology are unsupported
//    3). For unsupported processors the result is "0",
//        and the return status is "ippStsNotSupportedCpu".
//    4). For supported processors the result is "0",
//        and the return status is "ippStsUnknownCacheSize".
//        if sizes of the cache is unknown.
//
*/
IPPAPI( IppStatus, ippGetMaxCacheSizeB, ( int* pSizeByte ) )

/*
//  Name:       ippGetNumCoresOnDie
//  Purpose:    to distinguish MultiCore processors from other
//  Returns:    number of cores
//
*/
IPPAPI( int, ippGetNumCoresOnDie,( void ))

#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif

#ifdef __cplusplus
}
#endif

#endif /* __IPPCORE_H__ */
/* ////////////////////////////// End of file /////////////////////////////// */
