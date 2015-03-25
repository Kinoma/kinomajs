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


int kinoma_ipp_lib_avc_init(int implementation)
{
	int result_implementation = implementation;

	if( implementation == FSK_ARCH_AUTO )
		implementation = FskHardwareGetARMCPU_All();

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
#if !TARGET_OS_ANDROID
			ippsZero_32s_universal	= ippsZero_32u_arm;					
#endif
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

