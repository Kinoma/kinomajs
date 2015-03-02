/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __ALIGN_H__
#define __ALIGN_H__

#ifdef __INTEL_COMPILER
#define __ALIGN_BYTES 16
#define __ALIGNED(a) (((a) + __ALIGN_BYTES - 1) & ~(__ALIGN_BYTES - 1))
#define __ALIGN __declspec(align(__ALIGN_BYTES))
#else
#define __ALIGNED(a) (a)
#define __ALIGN
#endif

#endif
