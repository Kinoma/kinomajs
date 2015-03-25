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

#if defined(KINOMA_AVC) 

int kinoma_ipp_lib_avc_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V6;
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

			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR;
#if TARGET_OS_LINUX && !TARGET_OS_ANDROID
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c;
#else
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR;
#endif
			ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR;
			ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR;

			ippiInterpolateBlock_H264_8u_P2P1R_universal = 						ippiInterpolateBlock_H264_8u_P2P1R;
			ippiInterpolateLuma_H264_8u_C1R_universal = 						ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R;

			//Init_AVC_Interpolation(FSK_ARCH_XSCALE);
			ippiInterpolateLuma_H264_8u_C1R_16x16_universal	=					ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateLuma_H264_8u_C1R_16x8_universal	=					ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateLuma_H264_8u_C1R_8x16_universal	=					ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateLuma_H264_8u_C1R_8x8_universal	=					ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateLuma_H264_8u_C1R_8x4_universal	=					ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateLuma_H264_8u_C1R_4x8_universal	=					ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateLuma_H264_8u_C1R_4x4_universal	=					ippiInterpolateLuma_H264_8u_C1R;

			//ippiDecodeExpGolombOne_H264_1u16s_universal = 						ippiDecodeExpGolombOne_H264_1u16s;
			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal = 				ippiDecodeExpGolombOne_H264_1u16s_unsigned_ipp;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal = 				ippiDecodeExpGolombOne_H264_1u16s_signed_ipp;
			ippiDecodeCAVLCCoeffs_H264_1u16s_universal = 						ippiDecodeCAVLCCoeffs_H264_1u16s;
			ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal = 				ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s;
			ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal =     ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR;
			ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal =   ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR;


			ippiHuffmanRunLevelTableInitAlloc_32s_universal = 					ippiHuffmanRunLevelTableInitAlloc_32s;
			ippiHuffmanTableFree_32s_universal = 								ippiHuffmanTableFree_32s;
			ippiHuffmanTableInitAlloc_32s_universal = 							ippiHuffmanTableInitAlloc_32s;

			ippiInterpolateLumaTop_H264_8u_C1R_universal = 						ippiInterpolateLumaTop_H264_8u_C1R;
			ippiInterpolateLumaBottom_H264_8u_C1R_universal = 					ippiInterpolateLumaBottom_H264_8u_C1R;
			ippiInterpolateChromaTop_H264_8u_C1R_universal = 					ippiInterpolateChromaTop_H264_8u_C1R;							
			ippiInterpolateChromaBottom_H264_8u_C1R_universal = 				ippiInterpolateChromaBottom_H264_8u_C1R;
			ippiInterpolateBlock_H264_8u_P3P1R_universal = 						ippiInterpolateBlock_H264_8u_P3P1R;
			ippiUniDirWeightBlock_H264_8u_C1R_universal = 						ippiUniDirWeightBlock_H264_8u_C1R;
			ippiBiDirWeightBlock_H264_8u_P2P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P2P1R;
			ippiBiDirWeightBlock_H264_8u_P3P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P3P1R;
			ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P3P1R;
			ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P2P1R;

			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R;
			ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntraMB_H264_16s8u_P2R;
			ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal = 		ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R;
			ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaIntraMB_H264_16s8u_C1R;
			ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R;
			ippiReconstructLumaInterMB_H264_16s8u_C1R_universal = 				ippiReconstructLumaInterMB_H264_16s8u_C1R;
			ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R;
			ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R;
			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R;
			ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R;
			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R;
			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R;
			ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R;
			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R;
			ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal =     ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R;
			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R;
			ippiReconstructChromaInterMB_H264_16s8u_P2R_universal = 			ippiReconstructChromaInterMB_H264_16s8u_P2R;
			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R;

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

			//bitstream
			Init_AVC_CAVLC();
			
			
#ifndef DROP_C_NO
			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal = 				ippiDecodeExpGolombOne_H264_1u16s_unsigned_c;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal = 				ippiDecodeExpGolombOne_H264_1u16s_signed_c;
#endif			
			ippiDecodeCAVLCCoeffs_H264_1u16s_universal = 						ippiDecodeCAVLCCoeffs_H264_1u16s_c_only;
			ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal = 				ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_only;

#ifdef __INTEL_IPP__	//***bnie 2/10/2009  only implemented by Intel
			ippiHuffmanRunLevelTableInitAlloc_32s_universal = 					ippiHuffmanRunLevelTableInitAlloc_32s_c;
			ippiHuffmanTableFree_32s_universal = 								ippiHuffmanTableFree_32s_c;
			ippiHuffmanTableInitAlloc_32s_universal = 							ippiHuffmanTableInitAlloc_32s_c;
#endif

			Init_AVC_Interpolation_C();
			Init_AVC_Deblocking_C(1);
			Init_AVC_Reconstruction_C();

			//
			ippiUniDirWeightBlock_H264_8u_C1R_universal = 						ippiUniDirWeightBlock_H264_8u_C1R_MSc;
			ippiBiDirWeightBlock_H264_8u_P2P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P2P1R_MSc;
			ippiBiDirWeightBlock_H264_8u_P3P1R_universal = 						ippiBiDirWeightBlock_H264_8u_P3P1R_MSc;
			ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c;
			ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal = 				ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c;

			
#ifndef DROP_HIGH_PROFILE
			ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c;
			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c;
			ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal = 		ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c;
			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal = 			ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c;
			ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal =     ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c;
			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c;
			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal = 			ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c;
#endif

			//UMC::IppLumaDeblocking[0]	= ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal;
			//UMC::IppLumaDeblocking[1]	= ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal;
			//UMC::IppChromaDeblocking[0]	= ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal;
			//UMC::IppChromaDeblocking[1]	= ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal;

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
			ippsZero_32s_universal	= ippsZero_32u_arm;					
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					

			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal	= 	ippiDecodeExpGolombOne_H264_1u16s_unsigned_arm;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal		= 	ippiDecodeExpGolombOne_H264_1u16s_signed_arm;
			Init_AVC_CAVLC();
			ippiDecodeCAVLCCoeffs_H264_1u16s_universal				= 	ippiDecodeCAVLCCoeffs_H264_1u16s_c;
			ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal		= 	ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c;
			
			Init_AVC_Interpolation_ARM_V5();
			Init_AVC_Deblocking_ARM_V5(1);
			Init_AVC_Reconstruction_ARM_V5();
			
			result_implementation = FSK_ARCH_ARM_V5;

		}
#endif
	
#ifdef SUPPORT_OPENMAX	//overwrite with openmax
	if( implementation >= FSK_ARCH_ARM_V6 )
	{
		kinoma_openmax_lib_avc_dec_init(implementation);
		result_implementation = FSK_ARCH_ARM_V6;
	}
#endif

	return result_implementation;
}

#endif

#if defined(KINOMA_MP4V_DEBLOCKING) 

int kinoma_ipp_lib_mp4v_deblocking_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V6;
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
			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR;
#if TARGET_OS_LINUX && !TARGET_OS_ANDROID
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c;
#else
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR;
#endif
			result_implementation = FSK_ARCH_XSCALE;
			break;
#endif

#ifdef __KINOMA_IPP__
		case FSK_ARCH_C:
		default: 
			Init_AVC_Deblocking_C(0);

			result_implementation = FSK_ARCH_C;
			break;
#endif
	}

#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM
		if( implementation >= FSK_ARCH_ARM_V5 )
		{
			Init_AVC_Deblocking_ARM_V5(0);
			result_implementation = FSK_ARCH_ARM_V5;
		}
#endif
	
	return result_implementation;
}
#endif

#if defined(KINOMA_MP4V) 
int kinoma_ipp_lib_mp4v_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V6;
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

			//ippi
			//MPEG4 Video
			ippiDCT8x8Inv_16s_C1I_universal						= ippiDCT8x8Inv_16s_C1I;						
			ippiDCT8x8Inv_4x4_16s_C1I_universal					= ippiDCT8x8Inv_4x4_16s_C1I;					
			ippiDCT8x8Inv_2x2_16s_C1I_universal					= ippiDCT8x8Inv_2x2_16s_C1I;					
			ippiDCT8x8Inv_DC_16s_C1I_universal					= ippiDCT8x8Inv_2x2_16s_C1I;					
			ippiDCT8x8Inv_16s8u_C1R_universal					= ippiDCT8x8Inv_16s8u_C1R;					
			ippiDCT8x8Inv_4x4_16s8u_C1R_universal				= ippiDCT8x8Inv_16s8u_C1R;					
			ippiDCT8x8Inv_2x2_16s8u_C1R_universal				= ippiDCT8x8Inv_16s8u_C1R;					
			ippiDCT8x8Inv_DC_16s8u_C1R_universal				= ippiDCT8x8Inv_16s8u_C1R;					

			ippiQuantInvInterInit_MPEG4_universal				= ippiQuantInvInterInit_MPEG4;				
			ippiQuantInvInterGetSize_MPEG4_universal			= ippiQuantInvInterGetSize_MPEG4;			
			ippiQuantInvIntraInit_MPEG4_universal				= ippiQuantInvIntraInit_MPEG4;				
			ippiQuantInvIntraGetSize_MPEG4_universal			= ippiQuantInvIntraGetSize_MPEG4;			

			ippiQuantInvIntra_MPEG4_16s_C1I_universal 			= ippiQuantInvIntra_MPEG4_16s_C1I; 			
			ippiAdd8x8_16s8u_C1IRS_universal 					= ippiAdd8x8_16s8u_C1IRS;					
			ippiOBMC8x8HP_MPEG4_8u_C1R_universal				= ippiOBMC8x8HP_MPEG4_8u_C1R;				
			ippiCopy8x8QP_MPEG4_8u_C1R_universal 				= ippiCopy8x8QP_MPEG4_8u_C1R; 				

			ippiOBMC8x8QP_MPEG4_8u_C1R_universal				= ippiOBMC8x8QP_MPEG4_8u_C1R;				
			ippiAverage8x8_8u_C1IR_universal					= ippiAverage8x8_8u_C1IR;					
			ippiAverage16x16_8u_C1IR_universal					= ippiAverage16x16_8u_C1IR;					

#ifndef DROP_GMC
			ippiWarpGetSize_MPEG4_universal						= ippiWarpGetSize_MPEG4;						
			ippiWarpChroma_MPEG4_8u_P2R_universal				= ippiWarpChroma_MPEG4_8u_P2R;				        
			ippiWarpLuma_MPEG4_8u_C1R_universal					= ippiWarpLuma_MPEG4_8u_C1R;					
			ippiWarpInit_MPEG4_universal						= ippiWarpInit_MPEG4;						
			ippiCalcGlobalMV_MPEG4_universal					= ippiCalcGlobalMV_MPEG4;					
			ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal	= ippiChangeSpriteBrightness_MPEG4_8u_C1IR;			
#endif

			ippiCopy8x8_8u_C1R_universal						= ippiCopy8x8_8u_C1R;						
			ippiCopy16x16_8u_C1R_universal						= ippiCopy16x16_8u_C1R;						
			ippiCopy8x8HP_8u_C1R_universal						= ippiCopy8x8HP_8u_C1R;						
			ippiCopy8x4HP_8u_C1R_universal						= ippiCopy8x4HP_8u_C1R;						
			ippiCopy16x8HP_8u_C1R_universal						= ippiCopy16x8HP_8u_C1R;						
			ippiCopy16x16HP_8u_C1R_universal					= ippiCopy16x16HP_8u_C1R;					
			ippiCopy16x8QP_MPEG4_8u_C1R_universal				= ippiCopy16x8QP_MPEG4_8u_C1R;				
			ippiCopy16x16QP_MPEG4_8u_C1R_universal				= ippiCopy16x16QP_MPEG4_8u_C1R;				

			ippiAdd8x8HP_16s8u_C1RS_universal					= ippiAdd8x8HP_16s8u_C1RS;					
			ippiReconstructCoeffsInter_MPEG4_1u16s_universal	= ippiReconstructCoeffsInter_MPEG4_1u16s;	
			ippiDecodeDCIntra_MPEG4_1u16s_universal				= ippiDecodeDCIntra_MPEG4_1u16s;				
			ippiDecodeCoeffsIntra_MPEG4_1u16s_universal 		= ippiDecodeCoeffsIntra_MPEG4_1u16s; 		
			//ippiReconstructCoeffsIntra_H263_1u16s_universal 	= ippiReconstructCoeffsIntra_H263_1u16s; 	
			//ippiReconstructCoeffsInter_H263_1u16s_universal 	= ippiReconstructCoeffsInter_H263_1u16s; 	

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

			//mp4v
			ippiDCT8x8Inv_16s_C1I_universal						=ippiDCT8x8Inv_16s_C1I_c;
			ippiDCT8x8Inv_4x4_16s_C1I_universal					= ippiDCT8x8Inv_4x4_16s_C1I_c;
			ippiDCT8x8Inv_2x2_16s_C1I_universal					= ippiDCT8x8Inv_2x2_16s_C1I_c;
			ippiDCT8x8Inv_DC_16s_C1I_universal					= ippiDCT8x8Inv_DC_16s_C1I_c;
			ippiDCT8x8Inv_16s8u_C1R_universal					=ippiDCT8x8Inv_16s8u_C1R_c;
			ippiDCT8x8Inv_4x4_16s8u_C1R_universal				=ippiDCT8x8Inv_4x4_16s8u_C1R_c;
			ippiDCT8x8Inv_2x2_16s8u_C1R_universal				=ippiDCT8x8Inv_2x2_16s8u_C1R_c;
			ippiDCT8x8Inv_DC_16s8u_C1R_universal				=ippiDCT8x8Inv_DC_16s8u_C1R_c;

			ippiQuantInvInterInit_MPEG4_universal				= ippiQuantInvInterInit_MPEG4_c;				
			ippiQuantInvInterGetSize_MPEG4_universal			= ippiQuantInvInterGetSize_MPEG4_c;			
			ippiQuantInvIntraInit_MPEG4_universal				= ippiQuantInvIntraInit_MPEG4_c;				
			ippiQuantInvIntraGetSize_MPEG4_universal			= ippiQuantInvIntraGetSize_MPEG4_c;			

			ippiQuantInvIntra_MPEG4_16s_C1I_universal 			= ippiQuantInvIntra_MPEG4_16s_C1I_c; 			
			ippiAdd8x8_16s8u_C1IRS_universal 					= ippiAdd8x8_16s8u_C1IRS_c;					
			ippiOBMC8x8HP_MPEG4_8u_C1R_universal				= ippiOBMC8x8HP_MPEG4_8u_C1R_c;				
			ippiCopy8x8QP_MPEG4_8u_C1R_universal 				= ippiCopy8x8QP_MPEG4_8u_C1R_c; 				

			ippiOBMC8x8QP_MPEG4_8u_C1R_universal				= ippiOBMC8x8QP_MPEG4_8u_C1R_c;				
			ippiAverage8x8_8u_C1IR_universal					= ippiAverage8x8_8u_C1IR_c;					
			ippiAverage16x16_8u_C1IR_universal					= ippiAverage16x16_8u_C1IR_c;					

#ifndef DROP_GMC
			ippiWarpInit_MPEG4_universal						= ippiWarpInit_MPEG4_c;						
			ippiWarpGetSize_MPEG4_universal						= ippiWarpGetSize_MPEG4_c;						
			ippiWarpChroma_MPEG4_8u_P2R_universal				= ippiWarpChroma_MPEG4_8u_P2R_c;				        
			ippiWarpLuma_MPEG4_8u_C1R_universal					= ippiWarpLuma_MPEG4_8u_C1R_c;					
			ippiCalcGlobalMV_MPEG4_universal					= ippiCalcGlobalMV_MPEG4_c;				
			ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal	= ippiChangeSpriteBrightness_MPEG4_8u_C1IR_c;			
#endif
			ippiCopy8x8_8u_C1R_universal						= ippiCopy8x8_8u_C1R_c;						
			ippiCopy16x16_8u_C1R_universal						= ippiCopy16x16_8u_C1R_c;						
			ippiCopy8x8HP_8u_C1R_universal						= ippiCopy8x8HP_8u_C1R_c;
			ippiCopy16x8HP_8u_C1R_universal						= ippiCopy16x8HP_8u_C1R_c;						
			ippiCopy8x4HP_8u_C1R_universal						= ippiCopy8x4HP_8u_C1R_c;						
			ippiCopy16x16HP_8u_C1R_universal					= ippiCopy16x16HP_8u_C1R_c;					
			ippiCopy16x8QP_MPEG4_8u_C1R_universal				= ippiCopy16x8QP_MPEG4_8u_C1R_c;				
			ippiCopy16x16QP_MPEG4_8u_C1R_universal				= ippiCopy16x16QP_MPEG4_8u_C1R_c;				

			ippiAdd8x8HP_16s8u_C1RS_universal					= ippiAdd8x8HP_16s8u_C1RS_c;					
			ippiReconstructCoeffsInter_MPEG4_1u16s_universal	= ippiReconstructCoeffsInter_MPEG4_1u16s_c;	
			ippiDecodeDCIntra_MPEG4_1u16s_universal				= ippiDecodeDCIntra_MPEG4_1u16s_c;				
			ippiDecodeCoeffsIntra_MPEG4_1u16s_universal 		= ippiDecodeCoeffsIntra_MPEG4_1u16s_c; 		
			//ippiReconstructCoeffsIntra_H263_1u16s_universal 	= ippiReconstructCoeffsIntra_H263_1u16s_c; 	
			//ippiReconstructCoeffsInter_H263_1u16s_universal 	= ippiReconstructCoeffsInter_H263_1u16s_c; 	

			result_implementation = FSK_ARCH_C;
			break;
#endif
	}

//***overwrite with arm optimized functions
#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM
		
		if( implementation >= FSK_ARCH_ARM_V4)
		{
			ippsZero_8u_universal	= ippsZero_8u_arm;	
			ippsSet_8u_universal	= ippsSet_8u_arm;	
			ippsZero_16s_universal	= ippsZero_16u_arm;				
			ippsZero_32s_universal	= ippsZero_32u_arm;					
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					
			ippiCopy8x8HP_8u_C1R_universal = ippiCopy8x8HP_8u_C1R_arm;
			ippiAdd8x8HP_16s8u_C1RS_universal = ippiAdd8x8HP_16s8u_C1RS_arm;
			
			result_implementation = FSK_ARCH_ARM_V5;
		}

		if( implementation >= FSK_ARCH_ARM_V5 )
		{
			//common

			////ippi
			////MPEG4 Video
			ippiDCT8x8Inv_16s_C1I_universal		=ippiDCT8x8Inv_16s_C1I_arm_v5;
			ippiDCT8x8Inv_4x4_16s_C1I_universal	=ippiDCT8x8Inv_4x4_16s_C1I_arm_v5;
			ippiDCT8x8Inv_2x2_16s_C1I_universal	=ippiDCT8x8Inv_2x2_16s_C1I_arm_v5;
			//ippiDCT8x8Inv_DC_16s_C1I_universal	=ippiDCT8x8Inv_DC_16s_C1I_arm;
			ippiDCT8x8Inv_16s8u_C1R_universal	=ippiDCT8x8Inv_16s8u_C1R_arm_v5;
			ippiDCT8x8Inv_4x4_16s8u_C1R_universal =ippiDCT8x8Inv_4x4_16s8u_C1R_arm_v5;
			ippiDCT8x8Inv_2x2_16s8u_C1R_universal =ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v5;
			//ippiDCT8x8Inv_DC_16s8u_C1R_universal =ippiDCT8x8Inv_DC_16s8u_C1R_arm;

			//ippsSynthPQMF_MP3_32s16s_universal = ippsSynthPQMF_MP3_32s16s_arm;
			//ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_arm;
			//ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_arm;

			//ippsVLCDecodeBlock_1u16s_universal = ippsVLCDecodeBlock_1u16s_arm;
			//ippsVLCDecodeFree_32s_universal = ippsVLCDecodeFree_32s_arm;
			//ippsVLCDecodeInitAlloc_32s_universal = ippsVLCDecodeInitAlloc_32s_arm;
			//ippsVLCDecodeOne_1u16s_universal = ippsVLCDecodeOne_1u16s_arm;
 

			//ippsMalloc_8u_universal = ippsMalloc_8u_c;		//ippsMalloc_8u_arm;		
			//ippsFree_universal = ippsFree_c;				//ippsFree_arm;			
			
			//ippiSet_8u_C1R_universal = ippiSet_8u_C1R_c;	//ippiSet_8u_C1R_arm;		
			//ippiInterpolateChroma_H264_8u_C1R_universal = ippiInterpolateChroma_H264_8u_C1R_arm;
			//ippiInterpolateBlock_H264_8u_P3P1R_universal = ippiInterpolateBlock_H264_8u_P3P1R_arm;
			//ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal = ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_arm;

			result_implementation = FSK_ARCH_ARM_V5;
		}

#if defined(_WIN32_WCE) || TARGET_OS_ANDROID
		//over write with some v6 optimization
		if(implementation == FSK_ARCH_ARM_V6 )
		{
			////ippi
			////MPEG4 Video
			//ippiDCT8x8Inv_DC_16s_C1I_universal	=ippiDCT8x8Inv_DC_16s_C1I_arm;
			ippiDCT8x8Inv_16s_C1I_universal		  = ippiDCT8x8Inv_16s_C1I_arm_v6;
			ippiDCT8x8Inv_16s8u_C1R_universal	  = ippiDCT8x8Inv_16s8u_C1R_arm_v6;
			ippiDCT8x8Inv_4x4_16s_C1I_universal	  = ippiDCT8x8Inv_4x4_16s_C1I_arm_v6;
			ippiDCT8x8Inv_4x4_16s8u_C1R_universal = ippiDCT8x8Inv_4x4_16s8u_C1R_arm_v6;
			ippiDCT8x8Inv_2x2_16s_C1I_universal	  = ippiDCT8x8Inv_2x2_16s_C1I_arm_v6;
			ippiDCT8x8Inv_2x2_16s8u_C1R_universal = ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v6;

			result_implementation = FSK_ARCH_ARM_V6;
		}
#endif

#endif
	
	return result_implementation;
}

#endif

#if defined(KINOMA_AAC)
int kinoma_ipp_lib_aac_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V6;
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

			ippsSortAscend_32s_I_universal =					ippsSortAscend_32s_I;

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

			ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s;

			/*following functions are for 5.1  aac*/			
			ippsMul_32sc_Sfs_universal =					ippsMul_32sc_Sfs;		
			ippsMul_32s32sc_Sfs_universal =					ippsMul_32s32sc_Sfs;
			ippsMulC_32s_Sfs_universal =					ippsMulC_32s_Sfs;		


			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s;
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s;
#ifndef AAC_V6_0
			/* the following 4 functions used in MDCT*/			
			ippsFFTInitAlloc_C_32sc_universal =				ippsFFTInitAlloc_C_32sc;
			ippsFFTGetBufSize_C_32sc_universal =			ippsFFTGetBufSize_C_32sc;
			ippsFFTFree_C_32sc_universal =					ippsFFTFree_C_32sc;		
			ippsFFTFwd_CToC_32sc_Sfs_universal =			ippsFFTFwd_CToC_32sc_Sfs;

			ippsFFTInv_CToC_32s_Sfs_universal			= ippsFFTInv_CToC_32s_Sfs;
			ippsFFTInit_C_32s_universal					= ippsFFTInit_C_32s;		

			/* the following 6 functions exist in sbr code */  
			ippsFFTGetSize_C_32sc_universal =				ippsFFTGetSize_C_32sc;	
			ippsFFTInit_C_32sc_universal =					ippsFFTInit_C_32sc;		
			ippsFFTInv_CToC_32sc_Sfs_universal =			ippsFFTInv_CToC_32sc_Sfs;
			ippsFFTGetSize_C_32s_universal =				ippsFFTGetSize_C_32s;	
#endif

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
			
			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s_c;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s_c;
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s_c;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s_c;

			ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_c;

			/*following functions are for 5.1  aac*/			
			ippsMul_32sc_Sfs_universal =					ippsMul_32sc_Sfs_c;		
			ippsMul_32s32sc_Sfs_universal =					ippsMul_32s32sc_Sfs_c;
			ippsMulC_32s_Sfs_universal =					ippsMulC_32s_Sfs_c;		

			/* the following 4 functions used in MDCT*/			
#ifndef AAC_V6_0
			ippsFFTInitAlloc_C_32sc_universal =				ippsFFTInitAlloc_C_32sc_c;
			ippsFFTGetBufSize_C_32sc_universal =			ippsFFTGetBufSize_C_32sc_c;
			ippsFFTFree_C_32sc_universal =					ippsFFTFree_C_32sc_c;		
			ippsFFTFwd_CToC_32sc_Sfs_universal =			ippsFFTFwd_CToC_32sc_Sfs_c;

			ippsFFTInv_CToC_32s_Sfs_universal			= ippsFFTInv_CToC_32s_Sfs_c;
			ippsFFTInit_C_32s_universal					= ippsFFTInit_C_32s_c;		

			/* the following 6 functions exist in sbr code */  
			ippsFFTGetSize_C_32sc_universal =				ippsFFTGetSize_C_32sc_c;	
			ippsFFTInit_C_32sc_universal =					ippsFFTInit_C_32sc_c;		
			ippsFFTInv_CToC_32sc_Sfs_universal =			ippsFFTInv_CToC_32sc_Sfs_c;
			ippsFFTGetSize_C_32s_universal =				ippsFFTGetSize_C_32s_c;	
#endif
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
			ippsZero_32s_universal	= ippsZero_32u_arm;					
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					

			//ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_arm;
			//ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_arm;
			//ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_arm;

			//ippsVLCDecodeBlock_1u16s_universal		= ippsVLCDecodeBlock_1u16s_arm;
			//ippsVLCDecodeFree_32s_universal			= ippsVLCDecodeFree_32s_arm;
			//ippsVLCDecodeInitAlloc_32s_universal	= ippsVLCDecodeInitAlloc_32s_arm;
			//ippsVLCDecodeOne_1u16s_universal		= ippsVLCDecodeOne_1u16s_arm;
 

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

#endif

#if defined(KINOMA_MP3)
int kinoma_ipp_lib_mp3_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V6;
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

			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s;
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s;
			
  			ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s;
			ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s;

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
			
			ippsVLCDecodeBlock_1u16s_universal			= ippsVLCDecodeBlock_1u16s_c;
			ippsVLCDecodeFree_32s_universal				= ippsVLCDecodeFree_32s_c;
			ippsVLCDecodeInitAlloc_32s_universal		= ippsVLCDecodeInitAlloc_32s_c;
			ippsVLCDecodeOne_1u16s_universal			= ippsVLCDecodeOne_1u16s_c;

			ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_c;
			ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_c;


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
			ippsZero_32s_universal	= ippsZero_32u_arm;					
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					

			//ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_arm;
			//ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_arm;
			//ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_arm;

			//ippsVLCDecodeBlock_1u16s_universal		= ippsVLCDecodeBlock_1u16s_arm;
			//ippsVLCDecodeFree_32s_universal			= ippsVLCDecodeFree_32s_arm;
			//ippsVLCDecodeInitAlloc_32s_universal	= ippsVLCDecodeInitAlloc_32s_arm;
			//ippsVLCDecodeOne_1u16s_universal		= ippsVLCDecodeOne_1u16s_arm;
 

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

#endif

#if defined(KINOMA_AAC_ENC)

int kinoma_ipp_lib_aac_enc_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
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

#if defined(KINOMA_AAC_ENC)
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
#endif

#if defined(KINOMA_AAC_ENC)
			ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s;

			/*following functions are for 5.1  aac*/			
			ippsMul_32sc_Sfs_universal =					ippsMul_32sc_Sfs;		
			ippsMul_32s32sc_Sfs_universal =					ippsMul_32s32sc_Sfs;
			ippsMulC_32s_Sfs_universal =					ippsMulC_32s_Sfs;		

			/* the following 4 functions used in MDCT*/			
			//ippsFFTInitAlloc_C_32sc_universal =				ippsFFTInitAlloc_C_32sc;
			//ippsFFTGetBufSize_C_32sc_universal =			ippsFFTGetBufSize_C_32sc;
			//ippsFFTFree_C_32sc_universal =					ippsFFTFree_C_32sc;		
			//ippsFFTFwd_CToC_32sc_Sfs_universal =			ippsFFTFwd_CToC_32sc_Sfs;
#endif

#if defined(KINOMA_AAC_ENC)
			ippsSet_16s_universal = 						ippsSet_16s; 			
			ippsSet_32s_universal = 						ippsSet_32s; 			
			ippsAbs_16s_I_universal = 						ippsAbs_16s_I; 		
			ippsAbs_16s_universal = 						ippsAbs_16s; 			
			ippsMinEvery_32s_I_universal = 					ippsMinEvery_32s_I; 	
			ippsMaxEvery_32s_I_universal = 					ippsMaxEvery_32s_I; 	
			ippsMinMax_16s_universal = 						ippsMinMax_16s; 		
			ippsSum_16s32s_Sfs_universal = 					ippsSum_16s32s_Sfs; 	
			ippsSum_32s_Sfs_universal = 					ippsSum_32s_Sfs; 		
			ippsAdd_16s_Sfs_universal = 					ippsAdd_16s_Sfs; 		
			
			ippsAdd_16s_ISfs_universal = 					ippsAdd_16s_ISfs; 		
			ippsAddC_16s_Sfs_universal = 					ippsAddC_16s_Sfs; 		
			ippsSub_16s_universal = 						ippsSub_16s; 			
			ippsSub_16s_ISfs_universal = 					ippsSub_16s_ISfs; 		
			ippsSub_16s_Sfs_universal = 					ippsSub_16s_Sfs; 		
																		
			ippsMul_16s_Sfs_universal = 					ippsMul_16s_Sfs; 		
			ippsMul_16s_ISfs_universal = 					ippsMul_16s_ISfs; 		
			ippsMul_16s32s_Sfs_universal = 				ippsMul_16s32s_Sfs; 	
			ippsMulC_16s_Sfs_universal = 					ippsMulC_16s_Sfs; 		
			ippsMul_16s_universal = 						ippsMul_16s; 			

			ippsDiv_16s_Sfs_universal = 					ippsDiv_16s_Sfs; 		
			ippsDiv_16s_ISfs_universal = 					ippsDiv_16s_ISfs; 		
			ippsDivC_16s_ISfs_universal = 				ippsDivC_16s_ISfs; 	
			ippsSpread_16s_Sfs_universal = 			    ippsSpread_16s_Sfs; 			
			ippsPow34_16s_Sfs_universal = 			    ippsPow34_16s_Sfs; 			
			ippsMagnitude_16sc_Sfs_universal = 		    ippsMagnitude_16sc_Sfs; 		
			ippsMagnitude_16s_Sfs_universal = 		    ippsMagnitude_16s_Sfs; 		
			ippsRShiftC_32s_universal = 				    ippsRShiftC_32s; 				
			ippsRShiftC_16s_universal = 				    ippsRShiftC_16s; 				
			ippsLShiftC_16s_I_universal = 			    ippsLShiftC_16s_I; 			

			ippsDotProd_16s32s32s_Sfs_universal = 	    ippsDotProd_16s32s32s_Sfs; 	
			ippsDotProd_16s32s_Sfs_universal = 		    ippsDotProd_16s32s_Sfs; 		
			ippsLn_32s16s_Sfs_universal = 			    ippsLn_32s16s_Sfs; 			
			ippsDeinterleave_16s_universal = 			    ippsDeinterleave_16s; 			

			ippsVLCEncodeBlock_16s1u_universal = 			ippsVLCEncodeBlock_16s1u; 			
			ippsVLCEncodeEscBlock_AAC_16s1u_universal = 	ippsVLCEncodeEscBlock_AAC_16s1u; 
			ippsVLCEncodeInitAlloc_32s_universal = 		ippsVLCEncodeInitAlloc_32s;

			ippsVLCEncodeFree_32s_universal = 			ippsVLCEncodeFree_32s; 			
			ippsVLCCountBits_16s32s_universal = 			ippsVLCCountBits_16s32s; 			
			ippsVLCCountEscBits_AAC_16s32s_universal = 	ippsVLCCountEscBits_AAC_16s32s; 	

			ippsThreshold_LT_16s_I_universal = 			ippsThreshold_LT_16s_I; 			
			ippsConjPack_16sc_universal = 				ippsConjPack_16sc; 				
			ippsConjCcs_16sc_universal = 					ippsConjCcs_16sc; 					

			//ippsFFTInitAlloc_R_32s_universal = 			ippsFFTInitAlloc_R_32s; 			
			//ippsFFTGetBufSize_R_32s_universal =  			ippsFFTGetBufSize_R_32s;  			
			ippsFFTGetBufSize_R_16s_universal =  			ippsFFTGetBufSize_R_16s;  			
			//ippsFFTGetBufSize_C_16sc_universal =  		ippsFFTGetBufSize_C_16sc;  		
			ippsMDCTFwdGetBufSize_16s_universal = 		ippsMDCTFwdGetBufSize_16s; 		
			ippsMDCTFwdFree_16s_universal = 				ippsMDCTFwdFree_16s; 				
			ippsMDCTFwdInitAlloc_16s_universal = 			ippsMDCTFwdInitAlloc_16s; 			
			ippsMDCTFwd_16s_Sfs_universal = 				ippsMDCTFwd_16s_Sfs; 				
			ippsFFTInitAlloc_R_16s_universal = 			ippsFFTInitAlloc_R_16s; 			
			//ippsFFTInitAlloc_C_16s_universal = 			ippsFFTInitAlloc_C_16s; 			
			//ippsFFTInitAlloc_C_16sc_universal = 			ippsFFTInitAlloc_C_16sc; 			
			ippsFFTFree_R_16s_universal =  				ippsFFTFree_R_16s;  				
			//ippsFFTFree_C_16s_universal = 				ippsFFTFree_C_16s; 				
			//ippsFFTFree_C_16sc_universal = 				ippsFFTFree_C_16sc; 				
			//ippsFFTFwd_CToC_16sc_Sfs_universal = 			ippsFFTFwd_CToC_16sc_Sfs; 			
			ippsFFTFwd_RToCCS_16s_Sfs_universal = 		ippsFFTFwd_RToCCS_16s_Sfs; 		
			//ippsFFTFwd_RToPack_32s_Sfs_universal = 		ippsFFTFwd_RToPack_32s_Sfs; 		
			//ippsFFTInv_PackToR_32s_Sfs_universal = 		ippsFFTInv_PackToR_32s_Sfs; 		
#endif


#if defined(KINOMA_AVC_ENC)
			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR;
#if TARGET_OS_LINUX && !TARGET_OS_ANDROID
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c;
#else
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR;
#endif
			ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR;
			ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR;

			ippiInterpolateBlock_H264_8u_P2P1R_universal = 						ippiInterpolateBlock_H264_8u_P2P1R;
			ippiInterpolateLuma_H264_8u_C1R_universal = 						ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R;

			ippiEncodeCoeffsCAVLC_H264_16s_universal = 						ippiEncodeCoeffsCAVLC_H264_16s;					
			ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal = 				ippiEncodeChromaDcCoeffsCAVLC_H264_16s; 			


			ippiTransformDequantLumaDC_H264_16s_C1I_universal =   		    ippiTransformDequantLumaDC_H264_16s_C1I;   		
			ippiTransformDequantChromaDC_H264_16s_C1I_universal =  			ippiTransformDequantChromaDC_H264_16s_C1I;  		
			ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal = 	ippiDequantTransformResidualAndAdd_H264_16s_C1I; 	
			ippiTransformQuantChromaDC_H264_16s_C1I_universal = 			ippiTransformQuantChromaDC_H264_16s_C1I; 			
			ippiTransformQuantResidual_H264_16s_C1I_universal =  			ippiTransformQuantResidual_H264_16s_C1I;  			
			ippiTransformQuantLumaDC_H264_16s_C1I_universal =  				ippiTransformQuantLumaDC_H264_16s_C1I;  			


			ippiResize_8u_C1R_universal = 								    ippiResize_8u_C1R; 								
			ippiSAD16x16_8u32s_universal = 									ippiSAD16x16_8u32s; 								
			ippiSAD8x8_8u32s_C1R_universal = 								ippiSAD8x8_8u32s_C1R; 								
			ippiSAD4x4_8u32s_universal = 									ippiSAD4x4_8u32s; 									
			ippiSAD16x16Blocks8x8_8u16u_universal = 						ippiSAD16x16Blocks8x8_8u16u; 						
			ippiSAD16x16Blocks4x4_8u16u_universal =  						ippiSAD16x16Blocks4x4_8u16u;  						
			ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal = 				ippiSumsDiff16x16Blocks4x4_8u16s_C1; 				
			ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal = 				    ippiSumsDiff8x8Blocks4x4_8u16s_C1; 				
			ippiGetDiff4x4_8u16s_C1_universal = 							ippiGetDiff4x4_8u16s_C1; 							
			ippiEdgesDetect16x16_8u_C1R_universal = 						ippiEdgesDetect16x16_8u_C1R; 						
#endif

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


#if defined(KINOMA_AAC_ENC)
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
#endif

#if defined(KINOMA_AAC_ENC)
			ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_c;

			/*following functions are for 5.1  aac*/			
			ippsMul_32sc_Sfs_universal =					ippsMul_32sc_Sfs_c;		
			ippsMul_32s32sc_Sfs_universal =					ippsMul_32s32sc_Sfs_c;
			ippsMulC_32s_Sfs_universal =					ippsMulC_32s_Sfs_c;		

			/* the following 4 functions used in MDCT*/			
			//ippsFFTInitAlloc_C_32sc_universal =				ippsFFTInitAlloc_C_32sc_c;
			//ippsFFTGetBufSize_C_32sc_universal =			ippsFFTGetBufSize_C_32sc_c;
			//ippsFFTFree_C_32sc_universal =					ippsFFTFree_C_32sc_c;		
			//ippsFFTFwd_CToC_32sc_Sfs_universal =			ippsFFTFwd_CToC_32sc_Sfs_c;
#endif

#if defined(KINOMA_AAC_ENC)
			ippsSet_16s_universal = 						ippsSet_16s_c; 			
			ippsSet_32s_universal = 						ippsSet_32s_c; 			
			ippsAbs_16s_I_universal = 						ippsAbs_16s_I_c; 		
			ippsAbs_16s_universal = 						ippsAbs_16s_c; 			
			ippsMinEvery_32s_I_universal = 					ippsMinEvery_32s_I_c; 	
			ippsMaxEvery_32s_I_universal = 					ippsMaxEvery_32s_I_c; 	
			ippsMinMax_16s_universal = 						ippsMinMax_16s_c; 		
			ippsSum_16s32s_Sfs_universal = 					ippsSum_16s32s_Sfs_c; 	
			ippsSum_32s_Sfs_universal = 					ippsSum_32s_Sfs_c; 		
			ippsAdd_16s_Sfs_universal = 					ippsAdd_16s_Sfs_c; 		

			ippsAdd_16s_ISfs_universal = 					ippsAdd_16s_ISfs_c; 		
			ippsAddC_16s_Sfs_universal = 					ippsAddC_16s_Sfs_c; 		
			ippsSub_16s_universal = 						ippsSub_16s_c; 			
			ippsSub_16s_ISfs_universal = 					ippsSub_16s_ISfs_c; 		
			ippsSub_16s_Sfs_universal = 					ippsSub_16s_Sfs_c; 		
																		
			ippsMul_16s_Sfs_universal = 					ippsMul_16s_Sfs_c; 		
			ippsMul_16s_ISfs_universal = 					ippsMul_16s_ISfs_c; 		
			ippsMul_16s32s_Sfs_universal = 				ippsMul_16s32s_Sfs_c; 	
			ippsMulC_16s_Sfs_universal = 					ippsMulC_16s_Sfs_c; 		
			ippsMul_16s_universal = 						ippsMul_16s_c; 			

			ippsDiv_16s_Sfs_universal = 					ippsDiv_16s_Sfs_c; 		
			ippsDiv_16s_ISfs_universal = 					ippsDiv_16s_ISfs_c; 		
			ippsDivC_16s_ISfs_universal = 				ippsDivC_16s_ISfs_c; 	
			ippsSpread_16s_Sfs_universal = 			    ippsSpread_16s_Sfs_c; 			
			ippsPow34_16s_Sfs_universal = 			    ippsPow34_16s_Sfs_c; 			
			ippsMagnitude_16sc_Sfs_universal = 		    ippsMagnitude_16sc_Sfs_c; 		
			ippsMagnitude_16s_Sfs_universal = 		    ippsMagnitude_16s_Sfs_c; 		
			ippsRShiftC_32s_universal = 				    ippsRShiftC_32s_c; 				
			ippsRShiftC_16s_universal = 				    ippsRShiftC_16s_c; 				
			ippsLShiftC_16s_I_universal = 			    ippsLShiftC_16s_I_c; 			

			ippsDotProd_16s32s32s_Sfs_universal = 	    ippsDotProd_16s32s32s_Sfs_c; 	
			ippsDotProd_16s32s_Sfs_universal = 		    ippsDotProd_16s32s_Sfs_c; 		
			ippsLn_32s16s_Sfs_universal = 			    ippsLn_32s16s_Sfs_c; 			
			ippsDeinterleave_16s_universal = 			    ippsDeinterleave_16s_c; 			


			ippsVLCEncodeBlock_16s1u_universal = 			ippsVLCEncodeBlock_16s1u_c; 			
			ippsVLCEncodeEscBlock_AAC_16s1u_universal = 	ippsVLCEncodeEscBlock_AAC_16s1u_c; 	
			ippsVLCEncodeInitAlloc_32s_universal = 		ippsVLCEncodeInitAlloc_32s_c; 		
			ippsVLCEncodeFree_32s_universal = 			ippsVLCEncodeFree_32s_c; 			
			ippsVLCCountBits_16s32s_universal = 			ippsVLCCountBits_16s32s_c; 			
			ippsVLCCountEscBits_AAC_16s32s_universal = 	ippsVLCCountEscBits_AAC_16s32s_c; 	

			ippsThreshold_LT_16s_I_universal = 			ippsThreshold_LT_16s_I_c; 			
			ippsConjPack_16sc_universal = 				ippsConjPack_16sc_c; 				
			ippsConjCcs_16sc_universal = 					ippsConjCcs_16sc_c; 					


			//ippsFFTInitAlloc_R_32s_universal = 			ippsFFTInitAlloc_R_32s_c; 			
			//ippsFFTGetBufSize_R_32s_universal =  			ippsFFTGetBufSize_R_32s_c;  			
			ippsFFTInitAlloc_R_16s_universal = 			ippsFFTInitAlloc_R_16s_c; 			
			ippsFFTFwd_RToCCS_16s_Sfs_universal = 		ippsFFTFwd_RToCCS_16s_Sfs_c; 		
			ippsFFTGetBufSize_R_16s_universal =  			ippsFFTGetBufSize_R_16s_c;  			
			ippsMDCTFwdGetBufSize_16s_universal = 		ippsMDCTFwdGetBufSize_16s_c; 		
			ippsMDCTFwdFree_16s_universal = 				ippsMDCTFwdFree_16s_c; 				
			ippsMDCTFwdInitAlloc_16s_universal = 			ippsMDCTFwdInitAlloc_16s_c; 			
			ippsMDCTFwd_16s_Sfs_universal = 				ippsMDCTFwd_16s_Sfs_c; 				
			//ippsFFTInitAlloc_C_16s_universal = 			ippsFFTInitAlloc_C_16s_c; 			
			//ippsFFTGetBufSize_C_16sc_universal =  		ippsFFTGetBufSize_C_16sc_c;  		
			//ippsFFTInitAlloc_C_16sc_universal = 			ippsFFTInitAlloc_C_16sc_c; 			
			ippsFFTFree_R_16s_universal =  				ippsFFTFree_R_16s_c;  				
			//ippsFFTFree_C_16s_universal = 				ippsFFTFree_C_16s_c; 				
			//ippsFFTFree_C_16sc_universal = 				ippsFFTFree_C_16sc_c; 				
			//ippsFFTFwd_		CToC_16sc_Sfs_universal = 			ippsFFTFwd_CToC_16sc_Sfs_c; 			
			//ippsFFTFwd_RToPack_32s_Sfs_universal = 		ippsFFTFwd_RToPack_32s_Sfs_c; 		
			//ippsFFTInv_PackToR_32s_Sfs_universal = 		ippsFFTInv_PackToR_32s_Sfs_c; 		
#endif

	result_implementation = FSK_ARCH_C;
			break;
#endif
	}

//***overwrite with arm optimized functions
#ifdef __KINOMA_IPP_ARM_V5__
		if( implementation >= FSK_ARCH_ARM_V5 )
		{

			//common
			ippsZero_8u_universal	= ippsZero_8u_arm;	
			ippsSet_8u_universal	= ippsSet_8u_arm;	
			ippsZero_16s_universal	= ippsZero_16u_arm;				
			ippsZero_32s_universal	= ippsZero_32u_arm;					
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					


			//ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_arm;
			//ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_arm;
			//ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_arm;

			//ippsVLCDecodeBlock_1u16s_universal		= ippsVLCDecodeBlock_1u16s_arm;
			//ippsVLCDecodeFree_32s_universal			= ippsVLCDecodeFree_32s_arm;
			//ippsVLCDecodeInitAlloc_32s_universal	= ippsVLCDecodeInitAlloc_32s_arm;
			//ippsVLCDecodeOne_1u16s_universal		= ippsVLCDecodeOne_1u16s_arm;
 

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

#endif

#if defined(KINOMA_MP4V_ENC)

int kinoma_ipp_lib_mp4v_enc_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
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
#if defined(KINOMA_MP4V_ENC)
			ippsSortAscend_32s_I_universal =					ippsSortAscend_32s_I;
#endif


#if defined(KINOMA_MP4V_ENC) 
			//ippi
			//MPEG4 Video
			ippiDCT8x8Inv_16s_C1I_universal						= ippiDCT8x8Inv_16s_C1I;						
			ippiDCT8x8Inv_16s8u_C1R_universal					= ippiDCT8x8Inv_16s8u_C1R;					

			ippiWarpGetSize_MPEG4_universal						= ippiWarpGetSize_MPEG4;						
			ippiQuantInvInterInit_MPEG4_universal				= ippiQuantInvInterInit_MPEG4;				
			ippiQuantInvInterGetSize_MPEG4_universal			= ippiQuantInvInterGetSize_MPEG4;			
			ippiQuantInvIntraInit_MPEG4_universal				= ippiQuantInvIntraInit_MPEG4;				
			ippiQuantInvIntraGetSize_MPEG4_universal			= ippiQuantInvIntraGetSize_MPEG4;			

			ippiQuantInvIntra_MPEG4_16s_C1I_universal 			= ippiQuantInvIntra_MPEG4_16s_C1I; 			
			ippiAdd8x8_16s8u_C1IRS_universal 					= ippiAdd8x8_16s8u_C1IRS;					
			ippiOBMC8x8HP_MPEG4_8u_C1R_universal				= ippiOBMC8x8HP_MPEG4_8u_C1R;				
			ippiCopy8x8QP_MPEG4_8u_C1R_universal 				= ippiCopy8x8QP_MPEG4_8u_C1R; 				

			ippiOBMC8x8QP_MPEG4_8u_C1R_universal				= ippiOBMC8x8QP_MPEG4_8u_C1R;				
			ippiAverage8x8_8u_C1IR_universal					= ippiAverage8x8_8u_C1IR;					
			ippiAverage16x16_8u_C1IR_universal					= ippiAverage16x16_8u_C1IR;					

			ippiCalcGlobalMV_MPEG4_universal					= ippiCalcGlobalMV_MPEG4;					
			ippiWarpChroma_MPEG4_8u_P2R_universal				= ippiWarpChroma_MPEG4_8u_P2R;				        
			ippiWarpLuma_MPEG4_8u_C1R_universal					= ippiWarpLuma_MPEG4_8u_C1R;					
			ippiWarpInit_MPEG4_universal						= ippiWarpInit_MPEG4;						

			ippiCopy8x8_8u_C1R_universal						= ippiCopy8x8_8u_C1R;						
			ippiCopy16x16_8u_C1R_universal						= ippiCopy16x16_8u_C1R;						
			ippiCopy8x8HP_8u_C1R_universal						= ippiCopy8x8HP_8u_C1R;						
			ippiCopy8x4HP_8u_C1R_universal						= ippiCopy8x4HP_8u_C1R;						
			ippiCopy16x8HP_8u_C1R_universal						= ippiCopy16x8HP_8u_C1R;						
			ippiCopy16x16HP_8u_C1R_universal					= ippiCopy16x16HP_8u_C1R;					
			ippiCopy16x8QP_MPEG4_8u_C1R_universal				= ippiCopy16x8QP_MPEG4_8u_C1R;				
			ippiCopy16x16QP_MPEG4_8u_C1R_universal				= ippiCopy16x16QP_MPEG4_8u_C1R;				
#endif

#if defined(KINOMA_MP4V_ENC) 
			ippiSAD16x16_8u32s_universal = 					ippiSAD16x16_8u32s; 				
			ippiSAD16x8_8u32s_C1R_universal =  				ippiSAD16x8_8u32s_C1R;  			
			ippiSAD8x8_8u32s_C1R_universal =  				ippiSAD8x8_8u32s_C1R;  			
			ippiMeanAbsDev16x16_8u32s_C1R_universal = 		ippiMeanAbsDev16x16_8u32s_C1R; 	
			ippiQuantIntraInit_MPEG4_universal = 			ippiQuantIntraInit_MPEG4; 		
			ippiQuantInterInit_MPEG4_universal =  			ippiQuantInterInit_MPEG4;  		
			ippsMalloc_32u_universal = 			 			ippsMalloc_32u; 			 		
			ippiQuantInterGetSize_MPEG4_universal =  		ippiQuantInterGetSize_MPEG4;  	
			ippiQuantIntraGetSize_MPEG4_universal =  		ippiQuantIntraGetSize_MPEG4;  	
			ippiQuantIntra_H263_16s_C1I_universal =  		ippiQuantIntra_H263_16s_C1I;  	
			ippiQuantIntra_MPEG4_16s_C1I_universal =  		ippiQuantIntra_MPEG4_16s_C1I;  	
			ippiDCT8x8Fwd_8u16s_C1R_universal =  			ippiDCT8x8Fwd_8u16s_C1R;  		
			ippiQuantInter_H263_16s_C1I_universal =  		ippiQuantInter_H263_16s_C1I;  	
			ippiDCT8x8Fwd_16s_C1I_universal =  				ippiDCT8x8Fwd_16s_C1I;  			
			ippiSubSAD8x8_8u16s_C1R_universal =  			ippiSubSAD8x8_8u16s_C1R;  		
			ippiFrameFieldSAD16x16_8u32s_C1R_universal =  	ippiFrameFieldSAD16x16_8u32s_C1R; 
			ippiSub8x8_8u16s_C1R_universal =  				ippiSub8x8_8u16s_C1R;  			
			ippiQuantInter_MPEG4_16s_C1I_universal =  		ippiQuantInter_MPEG4_16s_C1I;  	
			ippiDCT8x8Fwd_16s_C1R_universal =  				ippiDCT8x8Fwd_16s_C1R;  			
			ippsCopy_1u_universal =  						ippsCopy_1u;  					
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R;  				
			ippiFrameFieldSAD16x16_16s32s_C1R_universal =  	ippiFrameFieldSAD16x16_16s32s_C1R;
			ippiSub16x16_8u16s_C1R_universal =  			ippiSub16x16_8u16s_C1R;  			
			ippiQuantInvIntra_H263_16s_C1I_universal =  	ippiQuantInvIntra_H263_16s_C1I;  	
			ippiSqrDiff16x16_8u32s_universal =  			ippiSqrDiff16x16_8u32s;  			
			ippiSSD8x8_8u32s_C1R_universal =  				ippiSSD8x8_8u32s_C1R;  			
			ippiQuantInvInter_H263_16s_C1I_universal =  	ippiQuantInvInter_H263_16s_C1I;  	
			ippiQuantInvInter_MPEG4_16s_C1I_universal =  	ippiQuantInvInter_MPEG4_16s_C1I;  
			ippiEncodeCoeffsIntra_H263_16s1u_universal =  	ippiEncodeCoeffsIntra_H263_16s1u; 
			ippiEncodeDCIntra_H263_16s1u_universal =  		ippiEncodeDCIntra_H263_16s1u;  	
			ippiCountZeros8x8_16s_C1_universal =  			ippiCountZeros8x8_16s_C1;  		
			ippiEncodeCoeffsIntra_MPEG4_16s1u_universal =  	ippiEncodeCoeffsIntra_MPEG4_16s1u;
			ippiEncodeDCIntra_MPEG4_16s1u_universal =  		ippiEncodeDCIntra_MPEG4_16s1u;  	
			ippiEncodeCoeffsInter_H263_16s1u_universal =  	ippiEncodeCoeffsInter_H263_16s1u; 
			ippiEncodeCoeffsInter_MPEG4_16s1u_universal =  	ippiEncodeCoeffsInter_MPEG4_16s1u;

#endif

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

#if defined(KINOMA_MP4V_ENC)
			ippsSortAscend_32s_I_universal =				ippsSortAscend_32s_I_c;
#endif


#if defined(KINOMA_MP4V_ENC) 
			//mp4v
			ippiDCT8x8Inv_16s_C1I_universal						=ippiDCT8x8Inv_16s_C1I_c;
			ippiDCT8x8Inv_16s8u_C1R_universal					=ippiDCT8x8Inv_16s8u_C1R_c;

			ippiWarpGetSize_MPEG4_universal						= ippiWarpGetSize_MPEG4_c;						
			ippiQuantInvInterInit_MPEG4_universal				= ippiQuantInvInterInit_MPEG4_c;				
			ippiQuantInvInterGetSize_MPEG4_universal			= ippiQuantInvInterGetSize_MPEG4_c;			
			ippiQuantInvIntraInit_MPEG4_universal				= ippiQuantInvIntraInit_MPEG4_c;				
			ippiQuantInvIntraGetSize_MPEG4_universal			= ippiQuantInvIntraGetSize_MPEG4_c;			

			ippiQuantInvIntra_MPEG4_16s_C1I_universal 			= ippiQuantInvIntra_MPEG4_16s_C1I_c; 			
			ippiAdd8x8_16s8u_C1IRS_universal 					= ippiAdd8x8_16s8u_C1IRS_c;					
			ippiOBMC8x8HP_MPEG4_8u_C1R_universal				= ippiOBMC8x8HP_MPEG4_8u_C1R_c;				
			ippiCopy8x8QP_MPEG4_8u_C1R_universal 				= ippiCopy8x8QP_MPEG4_8u_C1R_c; 				

			ippiOBMC8x8QP_MPEG4_8u_C1R_universal				= ippiOBMC8x8QP_MPEG4_8u_C1R_c;				
			ippiAverage8x8_8u_C1IR_universal					= ippiAverage8x8_8u_C1IR_c;					
			ippiAverage16x16_8u_C1IR_universal					= ippiAverage16x16_8u_C1IR_c;					

			ippiCalcGlobalMV_MPEG4_universal					= ippiCalcGlobalMV_MPEG4_c;				
			ippiWarpChroma_MPEG4_8u_P2R_universal				= ippiWarpChroma_MPEG4_8u_P2R_c;				        
			ippiWarpLuma_MPEG4_8u_C1R_universal					= ippiWarpLuma_MPEG4_8u_C1R_c;					
			ippiWarpInit_MPEG4_universal						= ippiWarpInit_MPEG4_c;						

			ippiCopy8x8_8u_C1R_universal						= ippiCopy8x8_8u_C1R_c;						
			ippiCopy16x16_8u_C1R_universal						= ippiCopy16x16_8u_C1R_c;						
			ippiCopy8x8HP_8u_C1R_universal						= ippiCopy8x8HP_8u_C1R_c;
			ippiCopy16x8HP_8u_C1R_universal						= ippiCopy16x8HP_8u_C1R_c;						
			ippiCopy8x4HP_8u_C1R_universal						= ippiCopy8x4HP_8u_C1R_c;						
			ippiCopy16x16HP_8u_C1R_universal					= ippiCopy16x16HP_8u_C1R_c;					
			ippiCopy16x8QP_MPEG4_8u_C1R_universal				= ippiCopy16x8QP_MPEG4_8u_C1R_c;				
			ippiCopy16x16QP_MPEG4_8u_C1R_universal				= ippiCopy16x16QP_MPEG4_8u_C1R_c;				
#endif

#if defined(KINOMA_MP4V_ENC) 
			ippiSAD16x16_8u32s_universal = 					ippiSAD16x16_8u32s_c; 				
			ippiSAD16x8_8u32s_C1R_universal =  				ippiSAD16x8_8u32s_C1R_c;  			
			ippiSAD8x8_8u32s_C1R_universal =  				ippiSAD8x8_8u32s_C1R_c;  			
			ippiMeanAbsDev16x16_8u32s_C1R_universal = 		ippiMeanAbsDev16x16_8u32s_C1R_c; 	
			ippiQuantIntraInit_MPEG4_universal = 			ippiQuantIntraInit_MPEG4_c; 		
			ippiQuantInterInit_MPEG4_universal =  			ippiQuantInterInit_MPEG4_c;  		
			ippsMalloc_32u_universal = 			 			ippsMalloc_32u_c; 			 		
			ippiQuantInterGetSize_MPEG4_universal =  		ippiQuantInterGetSize_MPEG4_c;  	
			ippiQuantIntraGetSize_MPEG4_universal =  		ippiQuantIntraGetSize_MPEG4_c;  	
			ippiQuantIntra_H263_16s_C1I_universal =  		ippiQuantIntra_H263_16s_C1I_c;  	
			ippiQuantIntra_MPEG4_16s_C1I_universal =  		ippiQuantIntra_MPEG4_16s_C1I_c;  	
			ippiDCT8x8Fwd_8u16s_C1R_universal =  			ippiDCT8x8Fwd_8u16s_C1R_c;  		
			ippiQuantInter_H263_16s_C1I_universal =  		ippiQuantInter_H263_16s_C1I_c;  	
			ippiDCT8x8Fwd_16s_C1I_universal =  				ippiDCT8x8Fwd_16s_C1I_c;  			
			ippiSubSAD8x8_8u16s_C1R_universal =  			ippiSubSAD8x8_8u16s_C1R_c;  		
			ippiFrameFieldSAD16x16_8u32s_C1R_universal =  	ippiFrameFieldSAD16x16_8u32s_C1R_c; 
			ippiSub8x8_8u16s_C1R_universal =  				ippiSub8x8_8u16s_C1R_c;  			
			ippiQuantInter_MPEG4_16s_C1I_universal =  		ippiQuantInter_MPEG4_16s_C1I_c;  	
			ippiDCT8x8Fwd_16s_C1R_universal =  				ippiDCT8x8Fwd_16s_C1R_c;  			
			ippsCopy_1u_universal =  						ippsCopy_1u_c;  					
			ippiCopy_8u_C1R_universal =  					ippiCopy_8u_C1R_c;  				
			ippiFrameFieldSAD16x16_16s32s_C1R_universal =  	ippiFrameFieldSAD16x16_16s32s_C1R_c;
			ippiSub16x16_8u16s_C1R_universal =  			ippiSub16x16_8u16s_C1R_c;  			
			ippiQuantInvIntra_H263_16s_C1I_universal =  	ippiQuantInvIntra_H263_16s_C1I_c;  	
			ippiSqrDiff16x16_8u32s_universal =  			ippiSqrDiff16x16_8u32s_c;  			
			ippiSSD8x8_8u32s_C1R_universal =  				ippiSSD8x8_8u32s_C1R_c;  			
			ippiQuantInvInter_H263_16s_C1I_universal =  	ippiQuantInvInter_H263_16s_C1I_c;  	
			ippiQuantInvInter_MPEG4_16s_C1I_universal =  	ippiQuantInvInter_MPEG4_16s_C1I_c;  
			ippiEncodeCoeffsIntra_H263_16s1u_universal =  	ippiEncodeCoeffsIntra_H263_16s1u_c; 
			ippiEncodeDCIntra_H263_16s1u_universal =  		ippiEncodeDCIntra_H263_16s1u_c;  	
			ippiCountZeros8x8_16s_C1_universal =  			ippiCountZeros8x8_16s_C1_c;  		
			ippiEncodeCoeffsIntra_MPEG4_16s1u_universal =  	ippiEncodeCoeffsIntra_MPEG4_16s1u_c;
			ippiEncodeDCIntra_MPEG4_16s1u_universal =  		ippiEncodeDCIntra_MPEG4_16s1u_c;  	
			ippiEncodeCoeffsInter_H263_16s1u_universal =  	ippiEncodeCoeffsInter_H263_16s1u_c; 
			ippiEncodeCoeffsInter_MPEG4_16s1u_universal =  	ippiEncodeCoeffsInter_MPEG4_16s1u_c;
#endif

			result_implementation = FSK_ARCH_C;
			break;
#endif
	}

//***overwrite with arm optimized functions
#ifdef __KINOMA_IPP_ARM_V5__
		if( implementation >= FSK_ARCH_ARM_V5 )
		{

			//common
			ippsZero_8u_universal	= ippsZero_8u_arm;	
			ippsSet_8u_universal	= ippsSet_8u_arm;	
			ippsZero_16s_universal	= ippsZero_16u_arm;				
			ippsZero_32s_universal	= ippsZero_32u_arm;					
			//ippsZero_32sc_universal = ippsZero_32sc_arm;					

#if defined(KINOMA_MP4V_ENC) 
			ippiCopy8x8HP_8u_C1R_universal			= ippiCopy8x8HP_8u_C1R_arm;

			////ippi
			////MPEG4 Video
			ippiDCT8x8Inv_16s_C1I_universal		=ippiDCT8x8Inv_16s_C1I_arm_v5;
			ippiDCT8x8Inv_16s8u_C1R_universal	=ippiDCT8x8Inv_16s8u_C1R_arm_v5;
			ippiDCT8x8Inv_4x4_16s_C1I_universal	=ippiDCT8x8Inv_4x4_16s_C1I_arm_v5;
			ippiDCT8x8Inv_2x2_16s_C1I_universal	=ippiDCT8x8Inv_2x2_16s_C1I_arm_v5;
#endif

			//ippsSynthPQMF_MP3_32s16s_universal			= ippsSynthPQMF_MP3_32s16s_arm;
			//ippsVLCDecodeEscBlock_AAC_1u16s_universal	= ippsVLCDecodeEscBlock_AAC_1u16s_arm;
			//ippsVLCDecodeEscBlock_MP3_1u16s_universal	= ippsVLCDecodeEscBlock_MP3_1u16s_arm;

			//ippsVLCDecodeBlock_1u16s_universal		= ippsVLCDecodeBlock_1u16s_arm;
			//ippsVLCDecodeFree_32s_universal			= ippsVLCDecodeFree_32s_arm;
			//ippsVLCDecodeInitAlloc_32s_universal	= ippsVLCDecodeInitAlloc_32s_arm;
			//ippsVLCDecodeOne_1u16s_universal		= ippsVLCDecodeOne_1u16s_arm;
 

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

#endif

#if defined(KINOMA_AVC_ENC)

int kinoma_ipp_lib_avc_enc_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
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

			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR;
#if TARGET_OS_LINUX && !TARGET_OS_ANDROID
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c;
#else
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR;
#endif
			ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR;
			ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR;

			ippiInterpolateBlock_H264_8u_P2P1R_universal = 						ippiInterpolateBlock_H264_8u_P2P1R;
			ippiInterpolateLuma_H264_8u_C1R_universal = 						ippiInterpolateLuma_H264_8u_C1R;
			ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R;

			ippiEncodeCoeffsCAVLC_H264_16s_universal = 						ippiEncodeCoeffsCAVLC_H264_16s;					
			ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal = 				ippiEncodeChromaDcCoeffsCAVLC_H264_16s; 			


			ippiTransformDequantLumaDC_H264_16s_C1I_universal =   		    ippiTransformDequantLumaDC_H264_16s_C1I;   		
			ippiTransformDequantChromaDC_H264_16s_C1I_universal =  			ippiTransformDequantChromaDC_H264_16s_C1I;  		
			ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal = 	ippiDequantTransformResidualAndAdd_H264_16s_C1I; 	
			ippiTransformQuantChromaDC_H264_16s_C1I_universal = 			ippiTransformQuantChromaDC_H264_16s_C1I; 			
			ippiTransformQuantResidual_H264_16s_C1I_universal =  			ippiTransformQuantResidual_H264_16s_C1I;  			
#if TARGET_CPU_ARM
			ippiTransformQuantFwd4x4_H264_16s_C1_universal =  				ippiTransformQuantFwd4x4_H264_16s_C1_c;  			
#else
			ippiTransformQuantFwd4x4_H264_16s_C1_universal =  				ippiTransformQuantFwd4x4_H264_16s_C1;  			
#endif
			ippiTransformQuantLumaDC_H264_16s_C1I_universal =  				ippiTransformQuantLumaDC_H264_16s_C1I;  			

			ippiMeanAbsDev16x16_8u32s_C1R_universal = 						ippiMeanAbsDev16x16_8u32s_C1R; 	

			ippiResize_8u_C1R_universal = 								    ippiResize_8u_C1R; 								
			ippiSAD16x16_8u32s_universal = 									ippiSAD16x16_8u32s; 								
			ippiSAD16x8_8u32s_C1R_universal =								ippiSAD16x8_8u32s_C1R;
			ippiSAD8x8_8u32s_C1R_universal = 								ippiSAD8x8_8u32s_C1R; 								
			ippiSAD4x4_8u32s_universal = 									ippiSAD4x4_8u32s; 									
			ippiSAD16x16Blocks8x8_8u16u_universal = 						ippiSAD16x16Blocks8x8_8u16u; 						
			ippiSAD16x16Blocks4x4_8u16u_universal =  						ippiSAD16x16Blocks4x4_8u16u;  						
			ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal = 				ippiSumsDiff16x16Blocks4x4_8u16s_C1; 				
			ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal = 				    ippiSumsDiff8x8Blocks4x4_8u16s_C1; 				
			ippiGetDiff4x4_8u16s_C1_universal = 							ippiGetDiff4x4_8u16s_C1; 							
#if TARGET_CPU_ARM
			ippiSub4x4_8u16s_C1R_universal = 								ippiSub4x4_8u16s_C1R_c; 							
#else
			ippiSub4x4_8u16s_C1R_universal = 								ippiSub4x4_8u16s_C1R; 							
#endif
			ippiEdgesDetect16x16_8u_C1R_universal = 						ippiEdgesDetect16x16_8u_C1R; 						
			
			ippiSub8x8_8u16s_C1R_universal =  								ippiSub8x8_8u16s_C1R;  			
			ippiSSD8x8_8u32s_C1R_universal =  								ippiSSD8x8_8u32s_C1R;  			
			ippiCopy8x8_8u_C1R_universal   =								ippiCopy8x8_8u_C1R;						
			ippiCopy16x16_8u_C1R_universal =								ippiCopy16x16_8u_C1R;						
			ippiSqrDiff16x16_8u32s_universal =  							ippiSqrDiff16x16_8u32s;  			

			Init_AVC_Interpolation_XScale();
			Init_AVC_Deblocking_XScale(1);
			//Init_AVC_Reconstruction_C();

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

			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c;
			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 			ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c;

			ippiInterpolateBlock_H264_8u_P2P1R_universal = 						ippiInterpolateBlock_H264_8u_P2P1R_c;

#ifndef DROP_C
			ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c;
			ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = 		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c;
			ippiInterpolateLuma_H264_8u_C1R_universal = 						ippiInterpolateLuma_H264_8u_C1R_c;
#endif

			ippiInterpolateChroma_H264_8u_C1R_universal = 						ippiInterpolateChroma_H264_8u_C1R_c;

			ippiEncodeCoeffsCAVLC_H264_16s_universal = 						ippiEncodeCoeffsCAVLC_H264_16s_c;					
			ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal = 				ippiEncodeChromaDcCoeffsCAVLC_H264_16s_c; 			


			ippiTransformDequantLumaDC_H264_16s_C1I_universal =   		    ippiTransformDequantLumaDC_H264_16s_C1I_c;   		
			ippiTransformDequantChromaDC_H264_16s_C1I_universal =  			ippiTransformDequantChromaDC_H264_16s_C1I_c;  		
			ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal = 	ippiDequantTransformResidualAndAdd_H264_16s_C1I_c; 	
			ippiTransformQuantChromaDC_H264_16s_C1I_universal = 			ippiTransformQuantChromaDC_H264_16s_C1I_c; 			
			ippiTransformQuantResidual_H264_16s_C1I_universal =  			ippiTransformQuantResidual_H264_16s_C1I_c;  			
			ippiTransformQuantFwd4x4_H264_16s_C1_universal =  				ippiTransformQuantFwd4x4_H264_16s_C1_c;  			
			ippiTransformQuantLumaDC_H264_16s_C1I_universal =  				ippiTransformQuantLumaDC_H264_16s_C1I_c;  			

			ippiMeanAbsDev16x16_8u32s_C1R_universal = 						ippiMeanAbsDev16x16_8u32s_C1R_c; 	

			ippiResize_8u_C1R_universal = 								    ippiResize_8u_C1R_c; 								
			ippiSAD16x16_8u32s_universal = 									ippiSAD16x16_8u32s_c; 								
			ippiSAD16x8_8u32s_C1R_universal =								ippiSAD16x8_8u32s_C1R_c;
			ippiSAD8x8_8u32s_C1R_universal = 								ippiSAD8x8_8u32s_C1R_c; 								
			ippiSAD4x4_8u32s_universal = 									ippiSAD4x4_8u32s_c; 									
			ippiSAD16x16Blocks8x8_8u16u_universal = 						ippiSAD16x16Blocks8x8_8u16u_c; 						
			ippiSAD16x16Blocks4x4_8u16u_universal =  						ippiSAD16x16Blocks4x4_8u16u_c;  						
			ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal = 				ippiSumsDiff16x16Blocks4x4_8u16s_C1_c; 				
			ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal = 				    ippiSumsDiff8x8Blocks4x4_8u16s_C1_c; 				
			ippiGetDiff4x4_8u16s_C1_universal = 							ippiGetDiff4x4_8u16s_C1_c; 							
			ippiSub4x4_8u16s_C1R_universal = 								ippiSub4x4_8u16s_C1R_c; 							
			ippiEdgesDetect16x16_8u_C1R_universal = 						ippiEdgesDetect16x16_8u_C1R_c; 						
			
			ippiSub8x8_8u16s_C1R_universal =  								ippiSub8x8_8u16s_C1R_c;  			
			ippiSSD8x8_8u32s_C1R_universal =  								ippiSSD8x8_8u32s_C1R_c;  			
			ippiCopy8x8_8u_C1R_universal   =								ippiCopy8x8_8u_C1R_c;						
			ippiCopy16x16_8u_C1R_universal =								ippiCopy16x16_8u_C1R_c;						
			ippiSqrDiff16x16_8u32s_universal =  							ippiSqrDiff16x16_8u32s_c;  			

			Init_AVC_Interpolation_C();
			Init_AVC_Deblocking_C(1);
			//Init_AVC_Reconstruction_C();

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
		ippsZero_32s_universal	= ippsZero_32u_arm;					
		
		Init_AVC_Interpolation_ARM_V5();
		Init_AVC_Deblocking_ARM_V5(1);
		//Init_AVC_Reconstruction_ARM_V5();
		
		result_implementation = FSK_ARCH_ARM_V5;
	}

	//some extra
	if( implementation >= FSK_ARCH_ARM_V6 )
	{
		ippiSAD16x16_8u32s_universal	= ippiSAD16x16_8u32s_arm_v6; 								
		ippiSAD8x8_8u32s_C1R_universal  = ippiSAD8x8_8u32s_arm_v6; 								
		ippiSAD4x4_8u32s_universal		= ippiSAD4x4_8u32s_arm_v6; 								
		
		ippiMeanAbsDev16x16_8u32s_C1R_universal	= ippiMeanAbsDev16x16_8u32s_C1R_arm_v6; 								
		
		result_implementation = FSK_ARCH_ARM_V6;
	}
#endif

#ifdef SUPPORT_OPENMAX	//overwrite with openmax
	if( implementation >= FSK_ARCH_ARM_V6 )
	{
		kinoma_openmax_lib_avc_enc_init(implementation);
		result_implementation = FSK_ARCH_ARM_V6;
	}
#endif
	
//***bnie: this has to be done in the end
#ifdef H264_ENC_V6_0
		Init_SAD_Procs();
#endif

	return result_implementation;
}

#endif

