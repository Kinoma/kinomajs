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


int kinoma_ipp_lib_avc_enc_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
		implementation = FskHardwareGetARMCPU_All();

	implementation = FSK_ARCH_C;
	
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
#if TARGET_OS_LINUX
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
		ippiSAD16x16_8u32s_universal	= ippiSAD16x16_8u32s_misaligned_arm_v6; 								
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



