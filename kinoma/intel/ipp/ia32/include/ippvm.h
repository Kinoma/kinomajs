/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
//              Intel(R) Integrated Performance Primitives
//                  Vector Math (ippvm)
//
*/

#if !defined( __IPPVM_H__ ) || defined( _OWN_BLDPCS )
#define __IPPVM_H__

#if defined (_WIN32_WCE) && defined (_M_IX86) && defined (__stdcall)
  #define _IPP_STDCALL_CDECL
  #undef __stdcall
#endif

#ifndef __IPPDEFS_H__
  #include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined( _OWN_BLDPCS )

#endif /* _OWN_BLDPCS */


/* /////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                   Functions declarations
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////// */


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippvmGetLibVersion
//  Purpose:    getting of the library version
//  Returns:    the structure of information about version
//              of ippVM library
//  Parameters:
//
//  Notes:      not necessary to release the returned structure
*/

IPPAPI( const IppLibraryVersion*, ippvmGetLibVersion, (void) )


IPPAPI( IppStatus, ippsInv_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInv_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInv_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInv_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInv_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsDiv_32f_A11, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsDiv_32f_A21, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsDiv_32f_A24, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsDiv_64f_A50, (const Ipp64f a[],const Ipp64f b[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsDiv_64f_A53, (const Ipp64f a[],const Ipp64f b[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsSqrt_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSqrt_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSqrt_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSqrt_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSqrt_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsInvSqrt_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvSqrt_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvSqrt_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvSqrt_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvSqrt_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsCbrt_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCbrt_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCbrt_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCbrt_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCbrt_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsInvCbrt_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvCbrt_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvCbrt_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvCbrt_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsInvCbrt_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsPow_32f_A11, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPow_32f_A21, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPow_32f_A24, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPow_64f_A50, (const Ipp64f a[],const Ipp64f b[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPow_64f_A53, (const Ipp64f a[],const Ipp64f b[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsPowx_32f_A11, (const Ipp32f a[],const Ipp32f b,Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPowx_32f_A21, (const Ipp32f a[],const Ipp32f b,Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPowx_32f_A24, (const Ipp32f a[],const Ipp32f b,Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPowx_64f_A50, (const Ipp64f a[],const Ipp64f b,Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsPowx_64f_A53, (const Ipp64f a[],const Ipp64f b,Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsExp_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsExp_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsExp_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsExp_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsExp_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsLn_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLn_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLn_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLn_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLn_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsLog10_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLog10_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLog10_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLog10_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsLog10_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsCos_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCos_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCos_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCos_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCos_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsSin_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSin_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSin_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSin_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSin_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsSinCos_32f_A11, (const Ipp32f a[],Ipp32f r1[],Ipp32f r2[],Ipp32s n))
IPPAPI( IppStatus, ippsSinCos_32f_A21, (const Ipp32f a[],Ipp32f r1[],Ipp32f r2[],Ipp32s n))
IPPAPI( IppStatus, ippsSinCos_32f_A24, (const Ipp32f a[],Ipp32f r1[],Ipp32f r2[],Ipp32s n))
IPPAPI( IppStatus, ippsSinCos_64f_A50, (const Ipp64f a[],Ipp64f r1[],Ipp64f r2[],Ipp32s n))
IPPAPI( IppStatus, ippsSinCos_64f_A53, (const Ipp64f a[],Ipp64f r1[],Ipp64f r2[],Ipp32s n))

IPPAPI( IppStatus, ippsTan_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTan_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTan_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTan_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTan_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsAcos_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcos_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcos_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcos_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcos_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsAsin_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsin_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsin_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsin_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsin_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsAtan_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsAtan2_32f_A11, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan2_32f_A21, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan2_32f_A24, (const Ipp32f a[],const Ipp32f b[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan2_64f_A50, (const Ipp64f a[],const Ipp64f b[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtan2_64f_A53, (const Ipp64f a[],const Ipp64f b[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsCosh_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCosh_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCosh_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCosh_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsCosh_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsSinh_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSinh_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSinh_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSinh_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsSinh_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsTanh_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTanh_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTanh_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTanh_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsTanh_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsAcosh_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcosh_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcosh_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcosh_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAcosh_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsAsinh_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsinh_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsinh_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsinh_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAsinh_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsAtanh_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtanh_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtanh_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtanh_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsAtanh_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsErf_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErf_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErf_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErf_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErf_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))

IPPAPI( IppStatus, ippsErfc_32f_A11, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErfc_32f_A21, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErfc_32f_A24, (const Ipp32f a[],Ipp32f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErfc_64f_A50, (const Ipp64f a[],Ipp64f r[],Ipp32s n))
IPPAPI( IppStatus, ippsErfc_64f_A53, (const Ipp64f a[],Ipp64f r[],Ipp32s n))


#ifdef __cplusplus
}
#endif

#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif

#endif /* __IPPVM_H__ */
/* ////////////////////////////// End of file /////////////////////////////// */

