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
#include "FskArch.h"

#ifdef SUPPORT_OPENMAX
#include "kinoma_openmax_lib.h"
#endif

IppStatus __STDCALL  ippiDCT8x8Fwd_16s_C1I_openmax_c(Ipp16s* pSrcDst);


int kinoma_ipp_lib_h263_enc_init(int implementation, int is_flv )
{
	int result_implementation = implementation;
	
	if( implementation == FSK_ARCH_AUTO )
		implementation = FskHardwareGetARMCPU_All();
	
	switch(implementation)
	{
#ifdef __INTEL_IPP__
		case FSK_ARCH_XSCALE:
			//common
			ippsMalloc_8u_universal						=	ippsMalloc_8u;			
			ippsMalloc_32u_universal					= ippsMalloc_32u; 			 		
			ippsFree_universal							=	ippsFree;				
			ippsZero_8u_universal						=	ippsZero_8u;			
			ippsZero_16s_universal						=	ippsZero_16s;		
			ippsZero_32s_universal						=	ippsZero_32s;		
			ippsZero_32sc_universal						=	ippsZero_32sc;		
			ippsCopy_8u_universal						=	ippsCopy_8u;			
			ippsSet_8u_universal						=	ippsSet_8u;				
			ippiCopy_8u_C1R_universal					=  	ippiCopy_8u_C1R;  				
			ippiSet_8u_C1R_universal					= 	ippiSet_8u_C1R;
			ippsMalloc_32s_universal					=	ippsMalloc_32s;		
			ippsCopy_32s_universal						=	ippsCopy_32s;		
			ippsCopy_16s_universal						= 	ippsCopy_16s; 			
			ippsSortAscend_32s_I_universal				=	ippsSortAscend_32s_I;

			//ippi
			//MPEG4 Video
			ippiDCT8x8Inv_16s_C1I_universal				= ippiDCT8x8Inv_16s_C1I;						
			ippiDCT8x8Inv_16s8u_C1R_universal			= ippiDCT8x8Inv_16s8u_C1R;					

			ippiAdd8x8_16s8u_C1IRS_universal 			= ippiAdd8x8_16s8u_C1IRS;					
			ippiOBMC8x8HP_MPEG4_8u_C1R_universal		= ippiOBMC8x8HP_MPEG4_8u_C1R;				

			ippiAverage8x8_8u_C1IR_universal			= ippiAverage8x8_8u_C1IR;					
			ippiAverage16x16_8u_C1IR_universal			= ippiAverage16x16_8u_C1IR;					

			ippiCopy8x8_8u_C1R_universal				= ippiCopy8x8_8u_C1R;						
			ippiCopy16x16_8u_C1R_universal				= ippiCopy16x16_8u_C1R;						
			ippiCopy8x8HP_8u_C1R_universal				= ippiCopy8x8HP_8u_C1R;						
			ippiCopy8x4HP_8u_C1R_universal				= ippiCopy8x4HP_8u_C1R;						
			ippiCopy16x8HP_8u_C1R_universal				= ippiCopy16x8HP_8u_C1R;						
			ippiCopy16x16HP_8u_C1R_universal			= ippiCopy16x16HP_8u_C1R;					

			ippiSAD16x16_8u32s_universal				= ippiSAD16x16_8u32s; 				
			ippiSAD8x8_8u32s_C1R_universal				= ippiSAD8x8_8u32s_C1R;  			
			ippiMeanAbsDev16x16_8u32s_C1R_universal		= ippiMeanAbsDev16x16_8u32s_C1R; 	
			ippiQuantIntra_H263_16s_C1I_universal		= ippiQuantIntra_H263_16s_C1I;  	
			ippiDCT8x8Fwd_8u16s_C1R_universal			= ippiDCT8x8Fwd_8u16s_C1R;  		
			ippiQuantInter_H263_16s_C1I_universal		= ippiQuantInter_H263_16s_C1I;  	
			ippiDCT8x8Fwd_16s_C1I_universal				= ippiDCT8x8Fwd_16s_C1I;  			
			ippiSubSAD8x8_8u16s_C1R_16x16_universal		= ippiSubSAD8x8_8u16s_C1R_16x16;  		
			ippiSubSAD8x8_8u16s_C1R_8x16_universal		= ippiSubSAD8x8_8u16s_C1R_8x16;  		
			ippiSub8x8_8u16s_C1R_universal				= ippiSub8x8_8u16s_C1R;  			
			ippiDCT8x8Fwd_16s_C1R_universal				= ippiDCT8x8Fwd_16s_C1R;  			
			ippsCopy_1u_universal						= ippsCopy_1u;  					
			ippiCopy_8u_C1R_universal					= ippiCopy_8u_C1R;  				
			ippiSub16x16_8u16s_C1R_universal			= ippiSub16x16_8u16s_C1R;  			
			ippiQuantInvIntra_H263_16s_C1I_universal	= ippiQuantInvIntra_H263_16s_C1I;  	
			ippiSqrDiff16x16_8u32s_universal			= ippiSqrDiff16x16_8u32s;  			
			ippiSSD8x8_8u32s_C1R_universal				= ippiSSD8x8_8u32s_C1R;  			
			ippiQuantInvInter_H263_16s_C1I_universal	= ippiQuantInvInter_H263_16s_C1I;  	
			ippiEncodeCoeffsIntra_H263_16s1u_universal	= ippiEncodeCoeffsIntra_H263_16s1u; 
			ippiEncodeDCIntra_H263_16s1u_universal		= ippiEncodeDCIntra_H263_16s1u;  	
			ippiCountZeros8x8_16s_C1_universal			= ippiCountZeros8x8_16s_C1;  		
			ippiEncodeCoeffsInter_H263_16s1u_universal	= ippiEncodeCoeffsInter_H263_16s1u; 

			result_implementation = FSK_ARCH_XSCALE;
			break;
#endif


#ifdef __KINOMA_IPP__
		case FSK_ARCH_C:
		default: 

			ippsMalloc_8u_universal						= ippsMalloc_8u_c;			
			ippsMalloc_32u_universal					= ippsMalloc_32u_c; 			 		
			ippsFree_universal							= ippsFree_c;				
			ippsZero_8u_universal						= ippsZero_8u_c;			
			ippsZero_16s_universal						= ippsZero_16s_c;		
			ippsZero_32s_universal						= ippsZero_32s_c;		
			ippsZero_32sc_universal						= ippsZero_32sc_c;		
			ippsCopy_8u_universal						= ippsCopy_8u_c;			
			ippsSet_8u_universal						= ippsSet_8u_c;				
			ippiCopy_8u_C1R_universal					= ippiCopy_8u_C1R_c;  				
			ippiSet_8u_C1R_universal					= ippiSet_8u_C1R_c;
			ippsMalloc_32s_universal					= ippsMalloc_32s_c;		
			ippsCopy_32s_universal						= ippsCopy_32s_c;		
			ippsCopy_16s_universal						= ippsCopy_16s_c; 			
			ippsSortAscend_32s_I_universal				= ippsSortAscend_32s_I_c;

			ippiAdd8x8_16s8u_C1IRS_universal 			= ippiAdd8x8_16s8u_C1IRS_c;					
			ippiAverage8x8_8u_C1IR_universal			= ippiAverage8x8_8u_C1IR_c;					
			ippiAverage16x16_8u_C1IR_universal			= ippiAverage16x16_8u_C1IR_c;					

			ippsCopy_1u_universal						= ippsCopy_1u_c;  					
			ippiCopy_8u_C1R_universal					= ippiCopy_8u_C1R_c;  				
			ippiCopy8x8_8u_C1R_universal				= ippiCopy8x8_8u_C1R_c;						
			ippiCopy16x16_8u_C1R_universal				= ippiCopy16x16_8u_C1R_c;						
			ippiCopy8x8HP_8u_C1R_universal				= ippiCopy8x8HP_8u_C1R_c;						
			ippiCopy8x4HP_8u_C1R_universal				= ippiCopy8x4HP_8u_C1R_c;						
			ippiCopy16x8HP_8u_C1R_universal				= ippiCopy16x8HP_8u_C1R_c;						
			ippiCopy16x16HP_8u_C1R_universal			= ippiCopy16x16HP_8u_C1R_c;					

			ippiCountZeros8x8_16s_C1_universal			= ippiCountZeros8x8_16s_C1_c;  

			ippiSAD16x16_8u32s_universal				= ippiSAD16x16_8u32s_c;		
			ippiSAD8x8_8u32s_C1R_universal				= ippiSAD8x8_8u32s_C1R_c;  			
			ippiSubSAD8x8_8u16s_C1R_16x16_universal		= ippiSubSAD8x8_8u16s_C1R_16x16_c;  		
			ippiSubSAD8x8_8u16s_C1R_8x16_universal		= ippiSubSAD8x8_8u16s_C1R_8x16_c;  		
			ippiSub8x8_8u16s_C1R_universal				= ippiSub8x8_8u16s_C1R_c;  			
			ippiSub16x16_8u16s_C1R_universal			= ippiSub16x16_8u16s_C1R_c;  			
			ippiMeanAbsDev16x16_8u32s_C1R_universal		= ippiMeanAbsDev16x16_8u32s_C1R_c; 	
			ippiSSD8x8_8u32s_C1R_universal				= ippiSSD8x8_8u32s_C1R_c;  			
			ippiSqrDiff16x16_8u32s_universal			= ippiSqrDiff16x16_8u32s_c;  			
			
			ippiDCT8x8Inv_16s_C1I_universal				= ippiDCT8x8Inv_16s_C1I_c;				
			ippiDCT8x8Inv_16s8u_C1R_universal			= ippiDCT8x8Inv_16s8u_C1R_c;				
			ippiDCT8x8Fwd_16s_C1I_universal				= ippiDCT8x8Fwd_16s_C1I_c;  		
			ippiDCT8x8Fwd_8u16s_C1R_universal			= ippiDCT8x8Fwd_8u16s_C1R_c;		
			
			ippiQuantIntra_H263_16s_C1I_universal		= ippiQuantIntra_H263_16s_C1I_c;  	
			ippiQuantInter_H263_16s_C1I_universal		= ippiQuantInter_H263_16s_C1I_c;  	
			ippiQuantInvIntra_H263_16s_C1I_universal	= ippiQuantInvIntra_H263_16s_C1I_c;  	
			ippiQuantInvInter_H263_16s_C1I_universal	= ippiQuantInvInter_H263_16s_C1I_c;  	
			ippiEncodeDCIntra_H263_16s1u_universal		= ippiEncodeDCIntra_H263_16s1u_c;  	
			ippiEncodeCoeffsInter_H263_16s1u_universal	= ippiEncodeCoeffsInter_H263_16s1u_c; 
			ippiEncodeCoeffsIntra_H263_16s1u_universal	= ippiEncodeCoeffsIntra_H263_16s1u_c; 

			result_implementation = FSK_ARCH_C;
			break;
#endif
	}

	if( is_flv )
	{
		ippiEncodeCoeffsInter_H263_16s1u_universal	= ippiEncodeCoeffsInter_H263_16s1u_flv_c; 
		ippiEncodeCoeffsIntra_H263_16s1u_universal	= ippiEncodeCoeffsIntra_H263_16s1u_flv_c; 
	}

//***overwrite with arm optimized functions
#ifdef __KINOMA_IPP_ARM_V5__
	if( implementation >= FSK_ARCH_ARM_V6 )
	{

		//common
		//ippsZero_8u_universal	= ippsZero_8u_arm;	
		//ippsSet_8u_universal	= ippsSet_8u_arm;	
		//ippsZero_16s_universal	= ippsZero_16u_arm;				
		//ippsZero_32s_universal	= ippsZero_32u_arm;					
		//ippsZero_32sc_universal = ippsZero_32sc_arm;					

		ippiCopy8x8HP_8u_C1R_universal			= ippiCopy8x8HP_8u_C1R_arm;
		
		ippiSAD16x16_8u32s_universal		    = ippiSAD16x16_8u32s_generic_arm_v6;		
		ippiSAD8x8_8u32s_C1R_universal			= ippiSAD8x8_8u32s_arm_v6;  			
		ippiMeanAbsDev16x16_8u32s_C1R_universal	= ippiMeanAbsDev16x16_8u32s_C1R_arm_v6; 	
		ippiSubSAD8x8_8u16s_C1R_16x16_universal	= ippiSubSAD8x8_8u16s_C1R_16x16_arm_v6;  		
		

		ippiDCT8x8Inv_16s_C1I_universal			= ippiDCT8x8Inv_16s_C1I_arm_v6;
		ippiDCT8x8Inv_16s8u_C1R_universal		= ippiDCT8x8Inv_16s8u_C1R_arm_v6;

		ippiAdd8x8_16s8u_C1IRS_universal 		= ippiAdd8x8_16s8u_C1IRS_arm_v6;					


		//ippsMalloc_8u_universal =  ippsMalloc_8u_c;		//ippsMalloc_8u_arm;		
		//ippsFree_universal =  ippsFree_c;				//ippsFree_arm;			
		
		//ippiSet_8u_C1R_universal = ippiSet_8u_C1R_c;	//ippiSet_8u_C1R_arm;		

		result_implementation = FSK_ARCH_ARM_V6;
	}
#endif

#ifdef SUPPORT_OPENMAX_NONONO	//overwrite with openmax
		ippiDCT8x8Fwd_16s_C1I_universal	= ippiDCT8x8Fwd_16s_C1I_openmax_c;  	
#endif
	
	return result_implementation;
}

#ifdef SUPPORT_OPENMAX_NONONO	//overwrite with openmax
#include "kinoma_utilities.h"
IppStatus __STDCALL  ippiDCT8x8Fwd_16s_C1I_openmax_c(Ipp16s* pSrcDst)
{
	__ALIGN16(Ipp16s, ttt, 64);

	omxVCM4P2_DCT8x8blk(pSrcDst, ttt);
	//ippiDCT8x8Fwd_16s_C1I_c(pSrcDst);
	memcpy( (void*)pSrcDst, (void *)ttt, 128);

	return (IppStatus)OMX_Sts_NoErr;
}
#endif
