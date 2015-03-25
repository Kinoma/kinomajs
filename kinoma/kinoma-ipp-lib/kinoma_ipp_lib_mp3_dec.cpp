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
//#include "Fsk.h"
#include "kinoma_ipp_lib.h"

#include "ippi.h"
#include "ippac.h"
#include "kinoma_ipp_common.h"
#include "FskPlatformImplementation.h"

#ifdef SUPPORT_OPENMAX
#include "kinoma_openmax_lib.h"
#endif

int kinoma_ipp_lib_mp3_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_IPHONE
		implementation = FskHardwareGetARMCPU_All();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V5;
#elif TARGET_OS_LINUX
		#if (__XSCALE__)
			implementation = FSK_ARCH_XSCALE;
		#elif TARGET_CPU_ARM
			implementation = FSK_ARCH_ARM_V5;
		#else
			implementation = FSK_ARCH_C;
		#endif
#else	
		implementation = FSK_ARCH_XSCALE;
#endif

	
	switch(implementation)
	{
#ifdef __INTEL_IPP__
		case FSK_ARCH_XSCALE:
			//common
			ippsMalloc_8u_universal =						ippsMalloc_8u;			
			ippsFree_universal =							ippsFree;				
			ippsZero_8u_universal =							ippsZero_8u;			
			ippsZero_16s_universal =						ippsZero_16s;		
			ippsZero_32s_universal =						ippsZero_32s;		
			ippsZero_32sc_universal =						ippsZero_32sc;		
			ippsCopy_8u_universal =							ippsCopy_8u;			
			ippsSet_8u_universal =							ippsSet_8u;				
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R;  				
			ippiSet_8u_C1R_universal = 						ippiSet_8u_C1R;
			ippsMalloc_32s_universal =						ippsMalloc_32s;		
			ippsCopy_32s_universal =						ippsCopy_32s;		
			ippsCopy_16s_universal = 						ippsCopy_16s; 			

			ippsSortAscend_32s_I_universal =				ippsSortAscend_32s_I;

			ippsAdd_32s_Sfs_universal =						ippsAdd_32s_Sfs;		
			ippsAdd_32s_ISfs_universal =					ippsAdd_32s_ISfs;	
			ippsMul_32s_ISfs_universal =					ippsMul_32s_ISfs;	
			ippsMul_32s_Sfs_universal =						ippsMul_32s_Sfs;		
			ippsSub_32s_Sfs_universal =						ippsSub_32s_Sfs;		
			ippsMinMax_32s_universal =						ippsMinMax_32s;		
			ippsMax_32s_universal =							ippsMax_32s;			
			ippsMax_16s_universal =							ippsMax_16s;			
			ippsMin_32s_universal =							ippsMin_32s;			
			ippsMinIndx_32s_universal =						ippsMinIndx_32s;			
			ippsMaxAbs_32s_universal =						ippsMaxAbs_32s;		
			ippsMove_32s_universal =						ippsMove_32s;		
			ippsDiv_32s_ISfs_universal =					ippsDiv_32s_ISfs;	
			ippsSqrt_64s_ISfs_universal =					ippsSqrt_64s_ISfs;	
			ippsConvert_64s32s_Sfs_universal =				ippsConvert_64s32s_Sfs;	
			ippsConvert_32s16s_Sfs_universal =				ippsConvert_32s16s_Sfs;	
			ippsLShiftC_32s_I_universal =					ippsLShiftC_32s_I;		
			ippsRShiftC_32s_I_universal =					ippsRShiftC_32s_I;		

			/* following functions only used in mp3*/			
			ippsAddC_32s_ISfs_universal =					ippsAddC_32s_ISfs;		
			ippsMulC_32s_ISfs_universal =					ippsMulC_32s_ISfs;

#ifndef KINOMA_FAST_HUFFMAN
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s;
			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s;
			ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s;
#endif

  			ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s;

			result_implementation = FSK_ARCH_XSCALE;
			break;
#endif


#ifdef __KINOMA_IPP__
		case FSK_ARCH_C:
		default: 

			//common
			ippsMalloc_8u_universal =						ippsMalloc_8u_c;			
			ippsFree_universal =							ippsFree_c;				
			ippsZero_8u_universal =							ippsZero_8u_c;			
			ippsZero_16s_universal =						ippsZero_16s_c;		
			ippsZero_32s_universal =						ippsZero_32s_c;		
			ippsZero_32sc_universal =						ippsZero_32sc_c;		
			ippsCopy_8u_universal =							ippsCopy_8u_c;			
			ippsSet_8u_universal =							ippsSet_8u_c;				
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R_c;  				
			ippiSet_8u_C1R_universal = 						ippiSet_8u_C1R_c;
			ippsMalloc_32s_universal =						ippsMalloc_32s_c;		
			ippsCopy_32s_universal =						ippsCopy_32s_c;		
			ippsCopy_16s_universal = 						ippsCopy_16s_c; 			

			ippsSortAscend_32s_I_universal =				ippsSortAscend_32s_I_c;

			ippsAdd_32s_Sfs_universal =						ippsAdd_32s_Sfs_c;		
			ippsAdd_32s_ISfs_universal =					ippsAdd_32s_ISfs_c;	
			ippsMul_32s_ISfs_universal =					ippsMul_32s_ISfs_c;	
			ippsMul_32s_Sfs_universal =						ippsMul_32s_Sfs_c;		
			ippsSub_32s_Sfs_universal =						ippsSub_32s_Sfs_c;		
			ippsMinMax_32s_universal =						ippsMinMax_32s_c;		
			ippsMax_32s_universal =							ippsMax_32s_c;			
			ippsMax_16s_universal =							ippsMax_16s_c;			
			ippsMin_32s_universal =							ippsMin_32s_c;			
			ippsMinIndx_32s_universal =						ippsMinIndx_32s_c;			
			ippsMaxAbs_32s_universal =						ippsMaxAbs_32s_c;		
			ippsMove_32s_universal =						ippsMove_32s_c;		
			ippsDiv_32s_ISfs_universal =					ippsDiv_32s_ISfs_c;	
			ippsSqrt_64s_ISfs_universal =					ippsSqrt_64s_ISfs_c;	
			ippsConvert_64s32s_Sfs_universal =				ippsConvert_64s32s_Sfs_c;	
			ippsConvert_32s16s_Sfs_universal =				ippsConvert_32s16s_Sfs_c;	
			ippsLShiftC_32s_I_universal =					ippsLShiftC_32s_I_c;		
			ippsRShiftC_32s_I_universal =					ippsRShiftC_32s_I_c;		

			/* following functions only used in mp3*/			
			ippsAddC_32s_ISfs_universal =					ippsAddC_32s_ISfs_c;		
			ippsMulC_32s_ISfs_universal =					ippsMulC_32s_ISfs_c;

#ifndef KINOMA_FAST_HUFFMAN			
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s_c;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s_c;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s_c;
			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s_c;
			ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_c;
#endif
			ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_c;


			result_implementation = FSK_ARCH_C;
			break;
#endif
	}

//***overwrite with arm optimized functions
#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM
		if( implementation >= FSK_ARCH_ARM_V5 )
		{

			//common
			ippsZero_8u_universal	= ippsZero_8u_arm;	
			ippsSet_8u_universal	= ippsSet_8u_arm;	
			ippsZero_16s_universal	= ippsZero_16u_arm;				
#if !TARGET_OS_ANDROID
			ippsZero_32s_universal	= ippsZero_32u_arm;					
#endif
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					

			//ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_arm;

#ifndef KINOMA_FAST_HUFFMAN
			ippsVLCDecodeInitAlloc_32s_universal	= ippsVLCDecodeInitAlloc_32s_arm;
			ippsVLCDecodeFree_32s_universal			= ippsVLCDecodeFree_32s_arm;
			ippsVLCDecodeOne_1u16s_universal		= ippsVLCDecodeOne_1u16s_arm;
			ippsVLCDecodeBlock_1u16s_universal		= ippsVLCDecodeBlock_1u16s_arm;
			ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_arm;
#endif

			//ippsMalloc_8u_universal =  ippsMalloc_8u_c;		//ippsMalloc_8u_arm;		
			//ippsFree_universal =  ippsFree_c;				//ippsFree_arm;			
			
			//ippiSet_8u_C1R_universal = ippiSet_8u_C1R_c;	//ippiSet_8u_C1R_arm;		
			//ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R_arm;
			//ippiInterpolateBlock_H264_8u_P3P1R_universal = 						ippiInterpolateBlock_H264_8u_P3P1R_arm;
			//ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_arm;

			result_implementation = FSK_ARCH_ARM_V5;

		}
#endif
	
	return result_implementation;
}




