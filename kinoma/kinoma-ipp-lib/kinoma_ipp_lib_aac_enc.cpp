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


int kinoma_ipp_lib_aac_enc_init(int implementation)
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
#ifdef _WIN32_WCE
		implementation = FskHardwareGetARMCPU();
#elif TARGET_OS_ANDROID
		implementation = FSK_ARCH_ARM_V5;	//***bnie:fix android 1/19/2011
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


