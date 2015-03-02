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
#include "kinoma_avc_defines.h"                                                                         
#include "ippvc.h"

#include "umc_media_data_ex.h"

#include "kinoma_ipp_lib.h"                                                                             
#include "kinoma_utilities.h"
#include "umc_h264_bitstream.h"
#include "umc_h264_dec_defs_dec.h"

using namespace UMC;

#define INVALID_SPS_PPS_ID  255

int kinoma_ipp_lib_avc_parse_init(int implementation)
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

			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal = 				ippiDecodeExpGolombOne_H264_1u16s_unsigned_ipp;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal = 				ippiDecodeExpGolombOne_H264_1u16s_signed_ipp;

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
			//Init_AVC_CAVLC();
			
			
#ifndef DROP_C_NO
			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal = 				ippiDecodeExpGolombOne_H264_1u16s_unsigned_c;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal = 				ippiDecodeExpGolombOne_H264_1u16s_signed_c;
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

			ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal	= 	ippiDecodeExpGolombOne_H264_1u16s_unsigned_arm;
			ippiDecodeExpGolombOne_H264_1u16s_signed_universal		= 	ippiDecodeExpGolombOne_H264_1u16s_signed_arm;
			
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

static int kinoma_avc_dec_get_spspps(MediaData3_V51 *pSource, H264SeqParamSet *sps, H264PicParamSet *pps)
{
        int                         i;
        int                             has_sps = 0;
        H264Bitstream   bs;
    	Status                 umcRes = UMC_OK;

    	for (i = 0; i < (Ipp32s) pSource->count; i++) {
                NAL_Unit_Type NALUType = (NAL_Unit_Type)pSource->nalu_type_ary[i];
                bs.Reset((Ipp8u *) (pSource->data_ptr_ary[i]), pSource->bit_offset_ary[i], (Ipp32u) (pSource->size_ary[i]));

        	if( NALUType == NAL_UT_SPS) {
                        umcRes = bs.GetSequenceParamSet(sps);
                        if (umcRes != UMC_OK)
                                return kFskErrBadData;

                        has_sps = 1;

                        if( pps == NULL )
                                break;
                } else if( NALUType == NAL_UT_PPS && has_sps ) {
            		// set illegal id
            		pps->pic_parameter_set_id = MAX_NUM_PIC_PARAM_SETS;
            		// Get id
            		umcRes = bs.GetPictureParamSetPart1(pps);
            		if (UMC_OK == umcRes) {
                		if (sps->seq_parameter_set_id >= MAX_NUM_SEQ_PARAM_SETS)
                    			return kFskErrBadData;

                		// Get rest of pic param set
                		umcRes = bs.GetPictureParamSetPart2(pps, sps);
                                	if (umcRes != UMC_OK)
                                        	return kFskErrBadData;
                        }

                        break;
                }
        }

	return kFskErrNone;
}


int kinoma_avc_dec_parse_header_info( int nalu_len_size, unsigned char *data, int size, int *left, int *right, int *top, int *bottom, int *frame_mbs_only_flag, int *profile, int *level )
{
	MediaData3_V51 *md = new MediaData3_V51(10);
	H264SeqParamSet sps;
	int width16, height16;
	int      err = kFskErrNone;

    if( md == NULL )
        return kFskErrMemFull;
    
	//***most likely called without being initialized!!!
	kinoma_ipp_lib_avc_parse_init(FSK_ARCH_AUTO);

	SwapInQTMediaSample3( 1, nalu_len_size, data, size, (void*)md );
	err = kinoma_avc_dec_get_spspps(md, &sps, NULL );

	width16  = 16 * ( sps.frame_width_in_mbs );
	height16 = 16 * ( sps.frame_height_in_mbs );

	if( left && right && top && bottom )
	{
		if( sps.frame_cropping_flag )
		{
			if( left   != NULL )	*left   = 2*sps.frame_cropping_rect_left_offset;
			if( right  != NULL )	*right  = width16 - (2*sps.frame_cropping_rect_right_offset+1);
			if( top    != NULL )	*top    = 2*sps.frame_cropping_rect_top_offset;
			if( bottom != NULL )	*bottom = height16 - (2*sps.frame_cropping_rect_bottom_offset+1);
		}
		else
		{
			if( left   != NULL )	*left   = 0;
			if( right  != NULL )	*right  = width16 - 1;
			if( top    != NULL )	*top    = 0;
			if( bottom != NULL )	*bottom = height16 - 1;
		}
	}

	if( frame_mbs_only_flag != NULL ) *frame_mbs_only_flag	= sps.frame_mbs_only_flag_kinoma_always_one;
	if( profile				!= NULL ) *profile				= sps.profile_idc;
	if( level				!= NULL ) *level				= sps.level_idc;
	
	if( md != NULL )
		delete md;

	return err;
}

int kinoma_avc_dec_parse_header_weight(unsigned char *data, int size, int *weighted_pred_flag )
{
	MediaData3_V51 *md = new MediaData3_V51(10);
	H264SeqParamSet sps;
	H264PicParamSet pps;
	int      err = kFskErrNone;
    
    if( md == NULL )
        return kFskErrMemFull;
    
	//***most likely called without being initialized!!!
	kinoma_ipp_lib_avc_parse_init(FSK_ARCH_AUTO);
	
	pps.pic_parameter_set_id = INVALID_SPS_PPS_ID;
	sps.seq_parameter_set_id = INVALID_SPS_PPS_ID;

	SwapInQTMediaSample3( 1, 4, data, size, (void*)md );
	err = kinoma_avc_dec_get_spspps(md, &sps, &pps );
	if( err != kFskErrNone || 
		sps.seq_parameter_set_id == INVALID_SPS_PPS_ID	||
		pps.pic_parameter_set_id == INVALID_SPS_PPS_ID )
		goto bail;
	
	if( weighted_pred_flag )
		*weighted_pred_flag = pps.weighted_pred_flag;

bail:
	if( md != NULL )
		delete md;
    
	return err;

}
