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
//#include "Fsk.h"
#include "kinoma_ipp_lib.h"

#include "ippi.h"
#include "ippac.h"
#include "kinoma_ipp_common.h"
#include "FskPlatformImplementation.h"

#ifdef SUPPORT_OPENMAX
#include "kinoma_openmax_lib.h"
#endif

int kinoma_ipp_lib_mp4v_enc_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V5;
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

