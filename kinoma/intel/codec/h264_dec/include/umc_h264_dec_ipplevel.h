/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/

#ifndef __UMC_H264_DEC_IPP_LEVEL_H
#define __UMC_H264_DEC_IPP_LEVEL_H

namespace IppLevel
{

#if defined( IPP_W32DLL )
#if defined( _MSC_VER ) || defined( __ICL ) || defined ( __ECL )
#define IPPFUN(type,name,arg) __declspec(dllexport) type __STDCALL name arg
#else
#define IPPFUN(type,name,arg)                extern type __STDCALL name arg
#endif
#else
#define   IPPFUN(type,name,arg)                extern type __STDCALL name arg
#endif

} // namespace IppLevel
using namespace IppLevel;

#endif // __UMC_H264_DEC_IPP_LEVEL_H
