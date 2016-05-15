/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#ifndef KINOMA_DEF_H
#define KINOMA_DEF_H

#ifdef KINOMA_AAC

#define ippsMalloc_8u					km_ippsMalloc_8u
#define ippsMalloc_32s					km_ippsMalloc_32s
#define ippsFree						km_ippsFree
#define ippsCopy_8u						km_ippsCopy_8u
#define ippsCopy_32s					km_ippsCopy_32s
#define ippsZero_8u						km_ippsZero_8u
#define ippsZero_16s					km_ippsZero_16s
#define ippsZero_32s					km_ippsZero_32s
#define ippsZero_32sc					km_ippsZero_32sc
#define ippsAdd_32s_Sfs					km_ippsAdd_32s_Sfs
#define ippsAdd_32s_ISfs				km_ippsAdd_32s_ISfs
#define ippsMul_32s_ISfs				km_ippsMul_32s_ISfs
#define ippsMul_32s_Sfs					km_ippsMul_32s_Sfs
#define ippsSub_32s_Sfs					km_ippsSub_32s_Sfs
#define ippsSortAscend_32s_I			km_ippsSortAscend_32s_I
#define ippsMinMax_32s					km_ippsMinMax_32s
#define ippsMax_32s						km_ippsMax_32s
#define ippsMax_16s						km_ippsMax_16s
#define ippsMin_32s						km_ippsMin_32s
#define ippsMaxAbs_32s					km_ippsMaxAbs_32s
#define ippsMove_32s					km_ippsMove_32s
#define ippsDiv_32s_ISfs				km_ippsDiv_32s_ISfs
#define ippsSqrt_64s_ISfs				km_ippsSqrt_64s_ISfs
#define ippsConvert_64s32s_Sfs			km_ippsConvert_64s32s_Sfs
#define ippsConvert_32s16s_Sfs			km_ippsConvert_32s16s_Sfs
#define ippsLShiftC_32s_I				km_ippsLShiftC_32s_I
#define ippsRShiftC_32s_I				km_ippsRShiftC_32s_I
#define ippsFFTGetBufSize_C_32sc		km_ippsFFTGetBufSize_C_32sc
#define ippsFFTInitAlloc_C_32sc			km_ippsFFTInitAlloc_C_32sc
#define ippsFFTFree_C_32sc				km_ippsFFTFree_C_32sc
#define ippsFFTFwd_CToC_32sc_Sfs		km_ippsFFTFwd_CToC_32sc_Sfs
#define ippsFFTInit_C_32sc				km_ippsFFTInit_C_32sc
#define ippsFFTGetSize_C_32sc			km_ippsFFTGetSize_C_32sc
#define ippsFFTInit_C_32s				km_ippsFFTInit_C_32s
#define ippsFFTGetSize_C_32s			km_ippsFFTGetSize_C_32s
#define ippsFFTInv_CToC_32s_Sfs			km_ippsFFTInv_CToC_32s_Sfs
#define ippsFFTInv_CToC_32sc_Sfs		km_ippsFFTInv_CToC_32sc_Sfs

#define ippsVLCDecodeInitAlloc_32s		km_ippsVLCDecodeInitAlloc_32s
#define ippsVLCDecodeFree_32s			km_ippsVLCDecodeFree_32s
#define ippsVLCDecodeOne_1u16s			km_ippsVLCDecodeOne_1u16s
#define ippsVLCDecodeBlock_1u16s		km_ippsVLCDecodeBlock_1u16s

#define ippsVLCDecodeEscBlock_AAC_1u16s	km_ippsVLCDecodeEscBlock_AAC_1u16s

/*for aac 5.1*/
#define ippsMul_32sc_Sfs				km_ippsMul_32sc_Sfs
#define ippsMul_32s32sc_Sfs				km_ippsMul_32s32sc_Sfs
#define ippsMulC_32s_Sfs				km_ippsMulC_32s_Sfs
#define ippsSet_8u						km_ippsSet_8u

#else
#ifdef KINOMA_MP3

#define ippsMalloc_8u					km_ippsMalloc_8u
#define ippsFree						km_ippsFree
#define ippsCopy_8u						km_ippsCopy_8u
#define ippsCopy_32s					km_ippsCopy_32s
#define ippsZero_8u						km_ippsZero_8u
#define ippsZero_32s					km_ippsZero_32s
#define ippsAdd_32s_Sfs					km_ippsAdd_32s_Sfs
#define ippsAddC_32s_ISfs				km_ippsAddC_32s_ISfs
#define ippsMulC_32s_ISfs				km_ippsMulC_32s_ISfs

#define ippsVLCDecodeInitAlloc_32s		km_ippsVLCDecodeInitAlloc_32s
#define ippsVLCDecodeFree_32s			km_ippsVLCDecodeFree_32s
#define ippsVLCDecodeOne_1u16s			km_ippsVLCDecodeOne_1u16s
#define ippsVLCDecodeBlock_1u16s		km_ippsVLCDecodeBlock_1u16s

#define ippsVLCDecodeEscBlock_MP3_1u16s	km_ippsVLCDecodeEscBlock_MP3_1u16s
#define ippsSynthPQMF_MP3_32s16s		km_ippsSynthPQMF_MP3_32s16s

/*for mp3 5.1 */
#define ippsZero_16s					km_ippsZero_16s

#endif

#endif



#include "ippac.h"
#include "ippdc.h"
#include "ipps.h"

#endif

