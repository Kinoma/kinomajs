/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef __KINOMA_IPP_LIB_UNIMPLEMENTED_H__
#define __KINOMA_IPP_LIB_UNIMPLEMENTED_H__

//***
#include "kinoma_ipp_env.h"
#include "FskArch.h"

#include "ippac.h"
#include "ippdc.h"
#include "ipps.h"
#include "ippvc.h"
#include "ippi.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VLC_SELF_ALLOCATE_MEMORY	1
#define MDCT_SELF_ALLOCATE_MEMORY	1

#if 0
#define ippsMDCTFwdInitAlloc_16s_xxx				ippsMDCTFwdInitAlloc_16s
#define ippsMDCTFwdGetBufSize_16s_xxx			ippsMDCTFwdGetBufSize_16s
#define ippsMDCTFwdFree_16s_xxx					ippsMDCTFwdFree_16s
#define ippsMDCTFwd_16s_Sfs_xxx					ippsMDCTFwd_16s_Sfs
#else
#define ippsMDCTFwdInitAlloc_16s_xxx				ippsMDCTFwdInitAlloc_16s_c
#define ippsMDCTFwdGetBufSize_16s_xxx			ippsMDCTFwdGetBufSize_16s_c
#define ippsMDCTFwdFree_16s_xxx					ippsMDCTFwdFree_16s_c
#define ippsMDCTFwd_16s_Sfs_xxx					ippsMDCTFwd_16s_Sfs_c

#endif

#if 0
#define ippsFFTInitAlloc_C_32sc_xx				ippsFFTInitAlloc_C_32sc_c	
#define ippsFFTFree_C_32sc_xx 					ippsFFTFree_C_32sc_c	
#define ippsFFTGetBufSize_C_32sc_xx				ippsFFTGetBufSize_C_32sc_c
#define ippsFFTFwd_CToC_32sc_Sfs_xx				ippsFFTFwd_CToC_32sc_Sfs_c	

#elif 1
#define ippsFFTInitAlloc_C_32sc_xx				ippsFFTInitAlloc_C_32sc_openmax	
#define ippsFFTFree_C_32sc_xx 					ippsFFTFree_C_32sc_openmax	
#define ippsFFTGetBufSize_C_32sc_xx				ippsFFTGetBufSize_C_32sc_openmax
#define ippsFFTFwd_CToC_32sc_Sfs_xx				ippsFFTFwd_CToC_32sc_Sfs_openmax	

#define ippsFFTInitAlloc_C_16sc_xx				ippsFFTInitAlloc_C_16sc_openmax	
#define ippsFFTFree_C_16sc_xx 					ippsFFTFree_C_16sc_openmax	
#define ippsFFTGetBufSize_C_16sc_xx				ippsFFTGetBufSize_C_16sc_openmax
#define ippsFFTFwd_CToC_16sc_Sfs_xx				ippsFFTFwd_CToC_16sc_Sfs_openmax	

#else
#define ippsFFTInitAlloc_C_32sc_xx				ippsFFTInitAlloc_C_32sc	
#define ippsFFTFree_C_32sc_xx 					ippsFFTFree_C_32sc	
#define ippsFFTGetBufSize_C_32sc_xx				ippsFFTGetBufSize_C_32sc
#define ippsFFTFwd_CToC_32sc_Sfs_xx				ippsFFTFwd_CToC_32sc_Sfs

#endif

#define FFT_SELF_ALLOCATE_MEMORY	1

#define ippMalloc_xxx							malloc//ippMalloc
#define ippFree_xxx								free//ippFree									

#ifdef __cplusplus
}
#endif


#endif	//__KINOMA_IPP_LIB_UNIMPLEMENTED_H__

