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
#include "FskArch.h"

#include "ippi.h"
#include "ippac.h"
#include "kinoma_ipp_common.h"
#include "FskPlatformImplementation.h"

#ifdef SUPPORT_OPENMAX_NOT_YET
#include "kinoma_openmax_lib.h"

#ifdef __cplusplus
extern "C" {
#endif
OMXResult omxVCM4P2_IDCT8x8blk (const OMX_S16 *pSrc, OMX_S16 *pDst);
#ifdef __cplusplus
}
#endif

IppStatus __STDCALL ippiDCT8x8Inv_16s_C1I_openmax_arm_v6 (Ipp16s *pSrcDst)
{
	__ALIGN16(Ipp16s, ttt, 64);

	omxVCM4P2_IDCT8x8blk(pSrcDst, ttt);
	memcpy( (void*)pSrcDst, (void *)ttt, 128);

	return (IppStatus)OMX_Sts_NoErr;
}
#endif


int kinoma_ipp_lib_mp4v_deblocking_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
		implementation = FskHardwareGetARMCPU_All();
	
	switch(implementation)
	{
#ifdef __INTEL_IPP__
		case FSK_ARCH_XSCALE:
			ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR;
#if TARGET_OS_LINUX
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

int kinoma_ipp_lib_mp4v_init(int implementation)
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
#if !TARGET_OS_ANDROID
			ippsZero_32s_universal	= ippsZero_32u_arm;					
#endif
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
#if TARGET_OS_ANDROID || defined(_WIN32_WCE)
		//over write with some v6 optimization
		if(implementation >= FSK_ARCH_ARM_V6 )
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
	
#ifdef SUPPORT_OPENMAX_NOT_YET	//overwrite with openmax
	ippiDCT8x8Inv_16s_C1I_universal	= ippiDCT8x8Inv_16s_C1I_openmax_arm_v6;

	//if(implementation >= FSK_ARCH_ARM_V6 )
	{
		//kinoma_openmax_lib_avc_enc_init(implementation);
		//result_implementation = FSK_ARCH_ARM_V6;
	}
#endif
	


	return result_implementation;
}


