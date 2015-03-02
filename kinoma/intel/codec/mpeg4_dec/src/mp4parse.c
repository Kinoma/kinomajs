/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    Parses MPEG-4 headers
//
*/
//***
#include "kinoma_ipp_lib.h"

#include "mp4.h"


mp4_Status mp4_Parse_VisualObject(mp4_Info* pInfo)
{
    mp4_VisualObject  *VO = &pInfo->VisualObject;

    VO->is_identifier = mp4_GetBit(pInfo);
    if (VO->is_identifier) {
        VO->verid = mp4_GetBits9(pInfo, 4);
        if ((VO->verid != 1) && (VO->verid != 2) && (VO->verid != 4) && (VO->verid != 5)) {
            mp4_Error("Warning: invalid version number in VO");
            VO->verid = 1;
        }
        VO->priority = mp4_GetBits9(pInfo, 3);
    } else
        VO->verid = 1;
    VO->type = mp4_GetBits9(pInfo, 4);
    if (VO->type != MP4_VISUAL_OBJECT_TYPE_VIDEO) {
        mp4_Error("Error: Unsupported object: visual_object_type != video ID");
        return MP4_STATUS_NOTSUPPORT;
    }
    VO->VideoSignalType.video_range = 0;
    VO->VideoSignalType.matrix_coefficients = MP4_VIDEO_COLORS_ITU_R_BT_709;
    if (VO->type == MP4_VISUAL_OBJECT_TYPE_VIDEO || VO->type == MP4_VISUAL_OBJECT_TYPE_TEXTURE) {
        VO->VideoSignalType.is_video_signal_type = mp4_GetBit(pInfo);
        if (VO->VideoSignalType.is_video_signal_type) {
            VO->VideoSignalType.video_format = mp4_GetBits9(pInfo, 3);
            VO->VideoSignalType.video_range = mp4_GetBit(pInfo);
            VO->VideoSignalType.is_colour_description = mp4_GetBit(pInfo);
            if (VO->VideoSignalType.is_colour_description) {
                VO->VideoSignalType.colour_primaries = mp4_GetBits9(pInfo, 8);
                VO->VideoSignalType.transfer_characteristics = mp4_GetBits9(pInfo, 8);
                VO->VideoSignalType.matrix_coefficients = mp4_GetBits9(pInfo, 8);
            }
        }
    }
    return MP4_STATUS_OK;
}

static mp4_Status mp4_Parse_QuantMatrix(mp4_Info* pInfo, Ipp8u pQM[64])
{
    Ipp32u  code;
    int     i;

    for (i = 0; i < 64; i ++) {
        code = mp4_GetBits9(pInfo, 8);
        if (code == 0) break;
            pQM[mp4_ClassicalZigzag[i]] = (Ipp8u)code;
    }
    code = pQM[mp4_ClassicalZigzag[i-1]];
    for (; i < 64; i ++) {
        pQM[mp4_ClassicalZigzag[i]] = (Ipp8u)code;
    }
    return MP4_STATUS_OK;
}

//***

mp4_Status mp4_Parse_VideoObject_original(mp4_Info* pInfo)
{
    Ipp32u  code;
    int     i;
    mp4_VisualObject  *VO = &pInfo->VisualObject;
    mp4_VideoObject  *VOL = &pInfo->VisualObject.VideoObject;

    code = mp4_ShowBits(pInfo, 32);
    // check short_video_start_marker
    //***
    if( ((code & (~0x3FF)) == 0x8000) ||
		((code & (~0x3FF)) == 0x8400)
	  )
	{
        VOL->short_video_header = 1;
        VOL->quant_precision = 5;
        VOL->shape = MP4_SHAPE_TYPE_RECTANGULAR;
        VOL->obmc_disable = 1;
        VOL->quant_type = 0;
        VOL->resync_marker_disable = 1;
        VOL->data_partitioned = 0;
        VOL->reversible_vlc = 0;
        VOL->VideoObjectPlane.rounding_type = 0;
        VOL->VideoObjectPlane.fcode_forward = 1;
        VOL->VideoObjectPlane.coded = 1;
        VOL->interlaced = 0;
        VOL->complexity_estimation_disable = 1;
        VOL->scalability = 0;
        VOL->not_8_bit = 0;
        VOL->bits_per_pixel = 8;
        VO->VideoSignalType.colour_primaries = 1;
        VO->VideoSignalType.transfer_characteristics = 1;
        VO->VideoSignalType.matrix_coefficients = 6;
        VO->VideoObject.VideoObjectPlaneH263.source_format = (pInfo->bufptr[4] >> 2) & 0x7;
        i = VO->VideoObject.VideoObjectPlaneH263.source_format - 1;
        if (i < 0 || i > 4) 
		{
			mp4_GetBits(pInfo, 24);	//***kinoma enhancement, skip it and keep going  --bnie  8/14/2008     
			mp4_Error("Error: Bad value for VideoPlaneWithShortHeader.source_format");
            return MP4_STATUS_PARSE_ERROR;
        }
        VOL->width = mp4_H263_width[i];
        VOL->height = mp4_H263_height[i];
        VOL->VideoObjectPlaneH263.num_gobs_in_vop = mp4_H263_gobvop[i];
        VOL->VideoObjectPlaneH263.num_macroblocks_in_gob = mp4_H263_mbgob[i];
        VOL->vop_time_increment_resolution = 30000;
        // VOL->VideoObjectPlane.time_increment = -1001;
        return MP4_STATUS_OK;
    }

#ifdef SUPPORT_H263_ONLY_AND_NOT_ESDS_PARSING			//***bnie: so that we can parse esds for ther mp4v decoders
	return MP4_STATUS_FILE_ERROR;
#else

    if (code < 256 + MP4_VIDEO_OBJECT_LAYER_MIN_SC || code > 256 + MP4_VIDEO_OBJECT_LAYER_MAX_SC) {
        mp4_Error("Error: Bad start code for VideoObjectLayer");
        return MP4_STATUS_PARSE_ERROR;
    }
    mp4_FlushBits(pInfo, 32);
    // video_object_start_code is founded
    VOL->id = code & 15;
    VOL->short_video_header = 0;
    VOL->random_accessible_vol = mp4_GetBit(pInfo);
    VOL->type_indication = mp4_GetBits9(pInfo, 8);
    if (VOL->type_indication != MP4_VIDEO_OBJECT_TYPE_SIMPLE &&
        VOL->type_indication != MP4_VIDEO_OBJECT_TYPE_CORE &&
        VOL->type_indication != MP4_VIDEO_OBJECT_TYPE_MAIN &&
        VOL->type_indication != MP4_VIDEO_OBJECT_TYPE_ADVANCED_REAL_TIME_SIMPLE &&
        VOL->type_indication != MP4_VIDEO_OBJECT_TYPE_ADVANCED_CODING_EFFICIENCY &&
        VOL->type_indication != MP4_VIDEO_OBJECT_TYPE_ADVANCED_SIMPLE) {
//f        mp4_Error("Error: Unsupported video_object");
//f        return MP4_STATUS_NOTSUPPORT;
    }
    VOL->is_identifier = mp4_GetBit(pInfo);
    if (VOL->is_identifier) {
        VOL->verid = mp4_GetBits9(pInfo, 4);
        if ((VOL->verid != 1) && (VOL->verid != 2) && (VOL->verid != 4) && (VOL->verid != 5)) {
            mp4_Error("Warning: invalid version number in VOL");
            VOL->verid = VO->verid;
        }
        VOL->priority = mp4_GetBits9(pInfo, 3);
    } else
        VOL->verid = VO->verid;
    VOL->aspect_ratio_info = mp4_GetBits9(pInfo, 4);
    if (VOL->aspect_ratio_info == MP4_ASPECT_RATIO_EXTPAR) {
        VOL->aspect_ratio_info_par_width = mp4_GetBits9(pInfo, 8);
        VOL->aspect_ratio_info_par_height = mp4_GetBits9(pInfo, 8);
    }
    VOL->is_vol_control_parameters = mp4_GetBit(pInfo);
    if (VOL->is_vol_control_parameters) {
        VOL->VOLControlParameters.chroma_format = mp4_GetBits9(pInfo, 2);
        if (VOL->VOLControlParameters.chroma_format != MP4_CHROMA_FORMAT_420) {
            mp4_Error("Error: vol_control_parameters.chroma_format != 4:2:0");
            return MP4_STATUS_PARSE_ERROR;
        }
        VOL->VOLControlParameters.low_delay = mp4_GetBit(pInfo);
        VOL->VOLControlParameters.vbv_parameters = mp4_GetBit(pInfo);
        if (VOL->VOLControlParameters.vbv_parameters) {
            VOL->VOLControlParameters.bit_rate = mp4_GetBits(pInfo, 15) << 15;
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOL->VOLControlParameters.bit_rate += mp4_GetBits(pInfo, 15);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            if (VOL->VOLControlParameters.bit_rate == 0) {
                mp4_Error("Error: vbv_parameters bit_rate == 0");
                return MP4_STATUS_PARSE_ERROR;
            }
            VOL->VOLControlParameters.vbv_buffer_size = mp4_GetBits(pInfo, 15) << 3;
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOL->VOLControlParameters.vbv_buffer_size += mp4_GetBits9(pInfo, 3);
            if (VOL->VOLControlParameters.vbv_buffer_size == 0) {
                mp4_Error("Error: vbv_parameters vbv_buffer_size == 0");
                return MP4_STATUS_PARSE_ERROR;
            }
            VOL->VOLControlParameters.vbv_occupancy = mp4_GetBits(pInfo, 11) << 15;
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOL->VOLControlParameters.vbv_occupancy += mp4_GetBits(pInfo, 15);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
        }
    }
    VOL->shape = mp4_GetBits9(pInfo, 2);
    if (VOL->shape != MP4_SHAPE_TYPE_RECTANGULAR) {
        mp4_Error("Error: video_object_layer_shape != rectangular (not supported)");
        return MP4_STATUS_PARSE_ERROR;
    }
    if (VOL->verid != 1 && VOL->shape == MP4_SHAPE_TYPE_GRAYSCALE) {
        VOL->shape_extension = mp4_GetBits9(pInfo, 4);
        if (VOL->shape_extension >= MP4_SHAPE_EXT_NUM) {
            mp4_Error("Error: wrong value for video_object_layer_shape_extension");
            return MP4_STATUS_PARSE_ERROR;
        }
    } else
        VOL->shape_extension = MP4_SHAPE_EXT_NUM;
    if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
    VOL->vop_time_increment_resolution = mp4_GetBits(pInfo, 16);
    if (VOL->vop_time_increment_resolution == 0) {
        mp4_Error("Error: wrong value for vop_time_increment_resolution");
        return MP4_STATUS_PARSE_ERROR;
    }
    if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
    // define number bits in vop_time_increment_resolution
    code = VOL->vop_time_increment_resolution - 1;
    i = 0;
    do {
        code >>= 1;
        i ++;
    } while (code);
    VOL->vop_time_increment_resolution_bits = i;
    VOL->fixed_vop_rate = mp4_GetBit(pInfo);
    if (VOL->fixed_vop_rate) {
        VOL->fixed_vop_time_increment = mp4_GetBits(pInfo, VOL->vop_time_increment_resolution_bits);
    }
    if (VOL->shape != MP4_SHAPE_TYPE_BINARYONLY) {
        if (VOL->shape == MP4_SHAPE_TYPE_RECTANGULAR) {
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOL->width = mp4_GetBits(pInfo, 13);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOL->height = mp4_GetBits(pInfo, 13);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
        }
        VOL->interlaced = mp4_GetBit(pInfo);
        VOL->obmc_disable = mp4_GetBit(pInfo);
        VOL->sprite_enable = mp4_GetBits9(pInfo, VOL->verid != 1 ? 2 : 1);
        if (VOL->sprite_enable == MP4_SPRITE_STATIC || VOL->sprite_enable == MP4_SPRITE_GMC) {
            if (VOL->sprite_enable == MP4_SPRITE_STATIC) {
                VOL->sprite_width = mp4_GetBits(pInfo, 13);
                if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
                VOL->sprite_height = mp4_GetBits(pInfo, 13);
                if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
                VOL->sprite_left_coordinate = mp4_GetBits(pInfo, 13);
                VOL->sprite_left_coordinate <<= (32 - 13);
                VOL->sprite_left_coordinate >>= (32 - 13);
                if (VOL->sprite_left_coordinate & 1) {
                    mp4_Error("Error: sprite_left_coordinate must be divisible by 2");
                    return MP4_STATUS_PARSE_ERROR;
                }
                if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
                VOL->sprite_top_coordinate = mp4_GetBits(pInfo, 13);
                VOL->sprite_top_coordinate <<= (32 - 13);
                VOL->sprite_top_coordinate >>= (32 - 13);
                if (VOL->sprite_top_coordinate & 1) {
                    mp4_Error("Error: sprite_top_coordinate must be divisible by 2");
                    return MP4_STATUS_PARSE_ERROR;
                }
                if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            }
            VOL->sprite_warping_points = mp4_GetBits9(pInfo, 6);
            if (VOL->sprite_warping_points > 4 ||
                (VOL->sprite_warping_points == 4 &&
                 VOL->sprite_enable == MP4_SPRITE_GMC)) {
                mp4_Error("Error: bad no_of_sprite_warping_points");
                return MP4_STATUS_PARSE_ERROR;
            }
            VOL->sprite_warping_accuracy = mp4_GetBits9(pInfo, 2);
            VOL->sprite_brightness_change = mp4_GetBit(pInfo);
            if (VOL->sprite_enable == MP4_SPRITE_GMC) {
                if (VOL->sprite_brightness_change) {
                    mp4_Error("Error: sprite_brightness_change should be 0 for GMC sprites");
                    return MP4_STATUS_PARSE_ERROR;
                }
            }
            if (VOL->sprite_enable != MP4_SPRITE_GMC) {
                VOL->low_latency_sprite_enable = mp4_GetBit(pInfo);
                if (VOL->low_latency_sprite_enable) {
                    mp4_Error("Error: low_latency_sprite not supported");
                    return MP4_STATUS_PARSE_ERROR;
                }
            }
        }
        if (VOL->verid != 1 && VOL->shape != MP4_SHAPE_TYPE_RECTANGULAR) {
            VOL->sadct_disable = mp4_GetBit(pInfo);
            if (!VOL->sadct_disable) {
                mp4_Error("Error: Shape Adaptive DCT is not supported");
                return MP4_STATUS_PARSE_ERROR;
            }
        }
        VOL->not_8_bit = mp4_GetBit(pInfo);
        if (VOL->not_8_bit) {
            mp4_Error("Error: only 8-bits data is supported");
            return MP4_STATUS_PARSE_ERROR;
        }
        if (VOL->not_8_bit) {
            VOL->quant_precision = mp4_GetBits9(pInfo, 4);
            if (VOL->quant_precision < 3 || VOL->quant_precision > 9) {
                mp4_Error("Error: quant_precision must be in range [3; 9]");
                return MP4_STATUS_PARSE_ERROR;
            }
            VOL->bits_per_pixel = mp4_GetBits9(pInfo, 4);
            if (VOL->bits_per_pixel < 4 || VOL->bits_per_pixel > 12) {
                mp4_Error("Error: bits_per_pixel must be in range [4; 12]");
                return MP4_STATUS_PARSE_ERROR;
            }
        } else {
            VOL->quant_precision = 5;
            VOL->bits_per_pixel = 8;
        }
        if (VOL->shape == MP4_SHAPE_TYPE_GRAYSCALE) {
            VOL->no_gray_quant_update = mp4_GetBit(pInfo);
            VOL->composition_method = mp4_GetBit(pInfo);
            VOL->linear_composition = mp4_GetBit(pInfo);
        }
        VOL->quant_type = mp4_GetBit(pInfo);
        if (VOL->quant_type) {
            VOL->load_intra_quant_mat = mp4_GetBit(pInfo);
            if (VOL->load_intra_quant_mat) {
                if (mp4_Parse_QuantMatrix(pInfo, VOL->intra_quant_mat) != MP4_STATUS_OK)
                    return MP4_STATUS_PARSE_ERROR;
            } else
                ippsCopy_8u_x(mp4_DefaultIntraQuantMatrix, VOL->intra_quant_mat, 64);
            VOL->load_nonintra_quant_mat = mp4_GetBit(pInfo);
            if (VOL->load_nonintra_quant_mat) {
                if (mp4_Parse_QuantMatrix(pInfo, VOL->nonintra_quant_mat) != MP4_STATUS_OK)
                    return MP4_STATUS_PARSE_ERROR;
            } else
                ippsCopy_8u_x(mp4_DefaultNonIntraQuantMatrix, VOL->nonintra_quant_mat, 64);
            if (VOL->shape == MP4_SHAPE_TYPE_GRAYSCALE) {
                int   ac, i;

                ac = mp4_aux_comp_count[VOL->shape_extension];
                for (i = 0; i < ac; i ++) {
                    VOL->load_intra_quant_mat_grayscale[i] = mp4_GetBit(pInfo);
                    if (VOL->load_intra_quant_mat_grayscale[i]) {
                        if (mp4_Parse_QuantMatrix(pInfo, VOL->intra_quant_mat_grayscale[i]) != MP4_STATUS_OK)
                            return MP4_STATUS_PARSE_ERROR;
                    } else
                        ippsCopy_8u_x(mp4_DefaultIntraQuantMatrix, VOL->intra_quant_mat_grayscale[i], 64);
                    VOL->load_nonintra_quant_mat_grayscale[i] = mp4_GetBit(pInfo);
                    if (VOL->load_nonintra_quant_mat_grayscale[i]) {
                        if (mp4_Parse_QuantMatrix(pInfo, VOL->nonintra_quant_mat_grayscale[i]) != MP4_STATUS_OK)
                            return MP4_STATUS_PARSE_ERROR;
                    } else
                        ippsCopy_8u_x(mp4_DefaultNonIntraQuantMatrix, VOL->nonintra_quant_mat_grayscale[i], 64);
                }
            }
        }
        if (VOL->verid != 1)
            VOL->quarter_sample = mp4_GetBit(pInfo);
        VOL->complexity_estimation_disable = mp4_GetBit(pInfo);
        if (!VOL->complexity_estimation_disable) {
            VOL->ComplexityEstimation.estimation_method = mp4_GetBits9(pInfo, 2);
            if (VOL->ComplexityEstimation.estimation_method <= 1) {
                VOL->ComplexityEstimation.shape_complexity_estimation_disable = mp4_GetBit(pInfo);
                if (!VOL->ComplexityEstimation.shape_complexity_estimation_disable) {
                    VOL->ComplexityEstimation.opaque =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.transparent =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.intra_cae =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.inter_cae =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.no_update =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.upsampling =  mp4_GetBit(pInfo);
                }
                VOL->ComplexityEstimation.texture_complexity_estimation_set_1_disable =  mp4_GetBit(pInfo);
                if (!VOL->ComplexityEstimation.texture_complexity_estimation_set_1_disable) {
                    VOL->ComplexityEstimation.intra_blocks =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.inter_blocks =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.inter4v_blocks =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.not_coded_blocks =  mp4_GetBit(pInfo);
                }
                if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
                VOL->ComplexityEstimation.texture_complexity_estimation_set_2_disable =  mp4_GetBit(pInfo);
                if (!VOL->ComplexityEstimation.texture_complexity_estimation_set_2_disable) {
                    VOL->ComplexityEstimation.dct_coefs =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.dct_lines =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.vlc_symbols =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.vlc_bits =  mp4_GetBit(pInfo);
                }
                VOL->ComplexityEstimation.motion_compensation_complexity_disable =  mp4_GetBit(pInfo);
                if (!VOL->ComplexityEstimation.motion_compensation_complexity_disable) {
                    VOL->ComplexityEstimation.apm =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.npm =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.interpolate_mc_q =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.forw_back_mc_q =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.halfpel2 =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.halfpel4 =  mp4_GetBit(pInfo);
                }
            }
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            if (VOL->ComplexityEstimation.estimation_method == 1) {
                // verid != 1
                VOL->ComplexityEstimation.version2_complexity_estimation_disable =  mp4_GetBit(pInfo);
                if (!VOL->ComplexityEstimation.version2_complexity_estimation_disable) {
                    VOL->ComplexityEstimation.sadct =  mp4_GetBit(pInfo);
                    VOL->ComplexityEstimation.quarterpel =  mp4_GetBit(pInfo);
                }
            }
        }
        VOL->resync_marker_disable = mp4_GetBit(pInfo);
        VOL->data_partitioned = mp4_GetBit(pInfo);
//f GrayScale Shapes does not support data_part
        if (VOL->data_partitioned)
            VOL->reversible_vlc = mp4_GetBit(pInfo);
        if (VOL->verid != 1) {
            VOL->newpred_enable = mp4_GetBit(pInfo);
            if (VOL->newpred_enable) {
                mp4_Error("Error: NEWPRED mode is not supported");
                return MP4_STATUS_PARSE_ERROR;
//f                VOL->requested_upstream_message_type = mp4_GetBits9(pInfo, 2);
//f                VOL->newpred_segment_type = mp4_GetBit(pInfo);
            }
            VOL->reduced_resolution_vop_enable = mp4_GetBit(pInfo);
            if (VOL->reduced_resolution_vop_enable) {
                mp4_Error("Error: Reduced Resolution VOP is not supported");
                return MP4_STATUS_PARSE_ERROR;
            }
        }
        VOL->scalability = mp4_GetBit(pInfo);
        if (VOL->scalability) {
            mp4_Error("Error: VOL scalability is not supported");
            return MP4_STATUS_PARSE_ERROR;
        }
    } else {
        if (VOL->verid != 1) {
            VOL->scalability = mp4_GetBit(pInfo);
            if (VOL->scalability) {
                mp4_Error("Error: VOL scalability is not supported");
                return MP4_STATUS_PARSE_ERROR;
            }
        }
        VOL->resync_marker_disable = mp4_GetBit(pInfo);
    }
    VOL->VideoObjectPlane.sprite_transmit_mode = MP4_SPRITE_TRANSMIT_MODE_PIECE;
#if 0
    {
        Ipp8u *sbp = pInfo->bufptr;
        int   v, b, i;
        if (mp4_SeekStartCodeValue(pInfo, MP4_USER_DATA_SC)) {
            if (pInfo->bufptr[0] == 'D' && pInfo->bufptr[1] == 'i' && pInfo->bufptr[2] == 'v' && pInfo->bufptr[3] == 'X') {
                pInfo->ftype = 2;
                v = (pInfo->bufptr[4] - '0') * 100 + (pInfo->bufptr[5] - '0') * 10 + (pInfo->bufptr[6] - '0');
                if (v < 503)
                    pInfo->ftype_f = 1;
                else if (v == 503) {
                    i = 8;
                    b = 0;
                    while (pInfo->bufptr[i] != 0) {
                        if (pInfo->bufptr[i] >= '0' && pInfo->bufptr[i] <= '9')
                            b = b * 10 + (pInfo->bufptr[i] - '0');
                        i ++;
                    }
                    pInfo->ftype_f = (b < 959) ? 1 : 0;
                } else
                    pInfo->ftype_f = 0;
            }
        }
        pInfo->bufptr = sbp;
    }
#else
    {
        Ipp8u *sbp = pInfo->bufptr;
        if (mp4_SeekStartCodeValue(pInfo, MP4_USER_DATA_SC)) {
            if (pInfo->bufptr[0] == 'D' && pInfo->bufptr[1] == 'i' && pInfo->bufptr[2] == 'v' && pInfo->bufptr[3] == 'X') {
                pInfo->ftype = 2;
                pInfo->ftype_f = 1;
            }
        }
        pInfo->bufptr = sbp;
    }
#endif
    return MP4_STATUS_OK;

#endif
}


//***
mp4_Status mp4_Parse_VideoObject_h263_plus(mp4_Info* pInfo)
{
	Ipp32u  code;
    mp4_VisualObject  *VO = &pInfo->VisualObject;
    mp4_VideoObject  *VOL = &pInfo->VisualObject.VideoObject;
    mp4_VideoObjectPlane     *VOP = &pInfo->VisualObject.VideoObject.VideoObjectPlane;
    mp4_VideoObjectPlaneH263 *VOPSH = &pInfo->VisualObject.VideoObject.VideoObjectPlaneH263;
	int marker_bit, zero_bit; //, four_reserved_zero_bits;
	int ufep;
    int custom_pcf=0;//***default value??? --bnie  8/24/09
    int umvplus = 0;
    //int somebits;
	int obmc;
	int h263_aic;
	int loop_filter;
	int unrestricted_mv;
	int h263_slice_structured=0;
	int alt_inter_vlc;
	int modified_quant;
	int no_rounding;

	//mp4_GetBits(pInfo, 22);
	VOPSH->temporal_reference = mp4_GetBits(pInfo, 8);
	marker_bit = mp4_GetBit(pInfo);
	zero_bit = mp4_GetBit(pInfo);
	VOPSH->split_screen_indicator = mp4_GetBit(pInfo);
	VOPSH->document_camera_indicator = mp4_GetBit(pInfo);
	VOPSH->full_picture_freeze_release = mp4_GetBit(pInfo);
	
	VOPSH->source_format = mp4_GetBits(pInfo, 3);
	ufep = mp4_GetBits(pInfo, 3);
	if( ufep == 1 )
	{
		VOPSH->source_format = mp4_GetBits(pInfo, 3);
		custom_pcf    = mp4_GetBit(pInfo);
		umvplus       = mp4_GetBit(pInfo); /* Unrestricted Motion Vector */
	    
		if( mp4_GetBit(pInfo) != 0 ) 
		{
			;//av_log(s->avctx, AV_LOG_ERROR, "Syntax-based Arithmetic Coding (SAC) not supported\n");
		}
	    
		obmc		= mp4_GetBit(pInfo);/* Advanced prediction mode */
		h263_aic	= mp4_GetBit(pInfo); /* Advanced Intra Coding (AIC) */
		loop_filter	= mp4_GetBit(pInfo);
		unrestricted_mv = umvplus || obmc || loop_filter;

		h263_slice_structured= mp4_GetBit(pInfo);
		if( mp4_GetBit(pInfo) != 0 ) 
		{
			;//av_log(s->avctx, AV_LOG_ERROR, "Reference Picture Selection not supported\n");
		}
		if( mp4_GetBit(pInfo) != 0 ) 
		{
			;//av_log(s->avctx, AV_LOG_ERROR, "Independent Segment Decoding not supported\n");
		}
	    
		alt_inter_vlc = mp4_GetBit(pInfo);
		modified_quant= mp4_GetBit(pInfo);
		//if( modified_quant )
		//	;//chroma_qscale_table = ff_h263_chroma_qscale_table;

		mp4_GetBit(pInfo); /* Prevent start code emulation */
		mp4_GetBits(pInfo, 3); /* Reserved */
    } 
	else if (ufep != 0) 
	{
        //av_log(s->avctx, AV_LOG_ERROR, "Bad UFEP type (%d)\n", ufep);
        return -1;
    }

    /* MPPTYPE */
    VOPSH->picture_coding_type = mp4_GetBits(pInfo, 3);
    switch(VOPSH->picture_coding_type)
	{
		case 0: 
			VOPSH->picture_coding_type= 0;//I_TYPE;
			break;
		case 1: 
			VOPSH->picture_coding_type= 1;//P_TYPE;
			break;
		case 3: 
			VOPSH->picture_coding_type= 2;//B_TYPE;
			break;
		case 7: 
			VOPSH->picture_coding_type= 0;//I_TYPE;
			break; //ZYGO
		default:
			return -1;
    }
	VOP->coding_type = VOPSH->picture_coding_type;

    mp4_GetBits(pInfo, 2);//skip 2 bits
    no_rounding = mp4_GetBit(pInfo);
    mp4_GetBits(pInfo, 4);

    /* Get the picture dimensions */
	if (ufep) 
	{
        if (VOPSH->source_format == 6) 
		{
            /* Custom Picture Format (CPFMT) */
           int aspect_ratio_info = mp4_GetBits(pInfo, 4);
            //dprintf("aspect: %d\n", s->aspect_ratio_info);
            /* aspect ratios:
            0 - forbidden
            1 - 1:1
            2 - 12:11 (CIF 4:3)
            3 - 10:11 (525-type 4:3)
            4 - 16:11 (CIF 16:9)
            5 - 40:33 (525-type 16:9)
            6-14 - reserved
            */
            VOL->width = (mp4_GetBits(pInfo, 9) + 1) * 4;
 			mp4_GetBit(pInfo);
            VOL->height = mp4_GetBits(pInfo, 9) * 4;
            //dprintf("\nH.263+ Custom picture: %dx%d\n",width,height);
            if( aspect_ratio_info == 15 )//FF_ASPECT_EXTENDED ) 
			{
                /* aspected dimensions */
                //int sample_aspect_ratio_num = mp4_GetBits(pInfo, 8);
                //int ample_aspect_ratio_den  = mp4_GetBits(pInfo, 8);
            }
			else
			{
                ;//s->avctx->sample_aspect_ratio= pixel_aspect[s->aspect_ratio_info];
            }
        } 
		else 
		{
			if(VOPSH->source_format == 1) /* SQCIF */
			{
				VOL->width  = 128;
				VOL->height = 96;
			}
			else if(VOPSH->source_format == 2) /* QCIF */
			{
				VOL->width  = 176;
				VOL->height = 144;
			}
			else if(VOPSH->source_format == 3) /* CIF */
			{
				VOL->width  = 352;
				VOL->height = 288;
			}
			else if(VOPSH->source_format == 4) /* 4CIF */
			{
				VOL->width  = 704;
				VOL->height = 576;
			}
			else if(VOPSH->source_format == 5) /* 16CIF */
			{
				VOL->width  = 1408;
				VOL->height = 1152;
			}
            //s->avctx->sample_aspect_ratio= (AVRational){12,11};
        }

        if(custom_pcf)
		{
            //int gcd;
            //s->avctx->time_base.den= 1800000;
            //s->avctx->time_base.num= 1000 + get_bits1(&s->gb);
            //s->avctx->time_base.num*= get_bits(&s->gb, 7);
            //if(s->avctx->time_base.num == 0){
            //    av_log(s, AV_LOG_ERROR, "zero framerate\n");
            //   return -1;
            //}
            //gcd= ff_gcd(s->avctx->time_base.den, s->avctx->time_base.num);
            //s->avctx->time_base.den /= gcd;
            //s->avctx->time_base.num /= gcd;
//                av_log(s->avctx, AV_LOG_DEBUG, "%d/%d\n", s->avctx->time_base.den, s->avctx->time_base.num);
        }
		else
		{
            //s->avctx->time_base= (AVRational){1001, 30000};
        }
	}

    if(custom_pcf)
        mp4_GetBits(pInfo, 2); //extended Temporal reference
 
    if (ufep) 
	{
        if(umvplus) 
		{
            if( mp4_GetBit(pInfo)==0 ) /* Unlimited Unrestricted Motion Vectors Indicator (UUI) */
                mp4_GetBit(pInfo);
        }
        if(h263_slice_structured)
		{
            if( mp4_GetBit(pInfo) != 0 ) 
			{
                ;//av_log(s->avctx, AV_LOG_ERROR, "rectangular slices not supported\n");
            }
            if( mp4_GetBit(pInfo) != 0) 
			{
                ;//av_log(s->avctx, AV_LOG_ERROR, "unordered slices not supported\n");
            }
        }
    }

    VOPSH->vop_quant = mp4_GetBits(pInfo, 5);

	{
		//int pei;

		for (;;) 
		{
			code = mp4_GetBit(pInfo); // pei
			if (!code)
				break;
			code = mp4_GetBits9(pInfo, 8); // psupp
		}
	}

	VOL->short_video_header = 1;
	VOL->quant_precision = 5;
	VOL->shape = MP4_SHAPE_TYPE_RECTANGULAR;
	VOL->obmc_disable = 1;
	VOL->quant_type = 0;
	VOL->resync_marker_disable = 1;
	VOL->data_partitioned = 0;
	VOL->reversible_vlc = 0;
	VOL->VideoObjectPlane.rounding_type = 0;
	VOL->VideoObjectPlane.fcode_forward = 1;
	VOL->VideoObjectPlane.coded = 1;
	VOL->interlaced = 0;
	VOL->complexity_estimation_disable = 1;
	VOL->scalability = 0;
	VOL->not_8_bit = 0;
	VOL->bits_per_pixel = 8;
	VO->VideoSignalType.colour_primaries = 1;
	VO->VideoSignalType.transfer_characteristics = 1;
	VO->VideoSignalType.matrix_coefficients = 6;
	VOL->VideoObjectPlaneH263.num_gobs_in_vop = 0;//***
	VOL->VideoObjectPlaneH263.num_macroblocks_in_gob = 0;//***
	VOL->vop_time_increment_resolution = 30000;
	// VOL->VideoObjectPlane.time_increment = -1001;
	return MP4_STATUS_OK;
}

mp4_Status mp4_Parse_VideoObject(mp4_Info* pInfo)
{
    Ipp32u  code;
    //int     i;
    mp4_VisualObject *VO  = &pInfo->VisualObject;
    mp4_VideoObject  *VOL = &pInfo->VisualObject.VideoObject;
	int		startcode;
	//int		marker_bit, zero_bit, four_reserved_zero_bits;
	int		temporal_reference; //, split_screen_indicator, document_camera_indicator;
	//int		full_picture_freeze_release;
	//int		source_format;
	//int		psupp, pei
	int		format;
	//int		droppable = 0;
	//int		deblockon;
	//int		qscale;
	//int		extrainfo;

    code = mp4_ShowBits(pInfo, 22);
	startcode = (code>>5);
	VOL->h263_flv = 1 + (code&0x0000001f);
	if( VOL->h263_flv <= 1 )
	{
		int  source_format = (pInfo->bufptr[4] >> 2) & 0x7;

		VOL->h263_flv_unlimited_UMV	= 0;
		if( source_format == 6 || source_format == 7 )
		{
			VOL->h263_plus = 1;
			mp4_GetBits(pInfo, 22);//skip start code and format code
			return mp4_Parse_VideoObject_h263_plus(pInfo);
		}
		else
		{
			VOL->h263_plus = 0;
			return mp4_Parse_VideoObject_original(pInfo);
		}
	}

	if( startcode != 1 )
		return -1;
	
	VOL->h263_flv_unlimited_UMV = 1;

    mp4_FlushBits(pInfo, 22);
    temporal_reference = mp4_GetBits9(pInfo, 8);
	format = mp4_GetBits9(pInfo, 3);
	if( format == 0 )
	{
		VOL->width  = mp4_GetBits9(pInfo, 8);
		VOL->height = mp4_GetBits9(pInfo, 8);
	}
	else if( format == 1 )
	{
		VOL->width  = mp4_GetBits(pInfo, 16);
		VOL->height = mp4_GetBits(pInfo, 16);
	}
	else if(  format == 2 )
	{
		VOL->width   = 352;
		VOL->height  = 288;
	}
	else if(  format == 3 )
	{
		VOL->width  = 176;
		VOL->height = 144;
	}
	else if(  format == 4 )
	{
		VOL->width  = 128;
		VOL->height = 96;
	}
	else if(  format == 5 )
	{
		VOL->width  = 320;
		VOL->height = 240;
	}
	else if(  format == 6 )
	{
		VOL->width  = 160;
		VOL->height = 120;
	}
	else
	{
		VOL->width  = 0;
		VOL->height = 0;
	}

    VOL->short_video_header = 1;
    VOL->quant_precision = 5;
    VOL->shape = MP4_SHAPE_TYPE_RECTANGULAR;
    VOL->obmc_disable = 1;
    VOL->quant_type = 0;
    VOL->resync_marker_disable = 1;
    VOL->data_partitioned = 0;
    VOL->reversible_vlc = 0;
    VOL->VideoObjectPlane.rounding_type = 0;
    VOL->VideoObjectPlane.fcode_forward = 1;
    VOL->VideoObjectPlane.coded = 1;
    VOL->interlaced = 0;
    VOL->complexity_estimation_disable = 1;
    VOL->scalability = 0;
    VOL->not_8_bit = 0;
    VOL->bits_per_pixel = 8;
    VO->VideoSignalType.colour_primaries = 1;
    VO->VideoSignalType.transfer_characteristics = 1;
    VO->VideoSignalType.matrix_coefficients = 6;
    VO->VideoObject.VideoObjectPlaneH263.source_format = format;
    //VOL->width = mp4_H263_width[i];
    //VOL->height = mp4_H263_height[i];
    VOL->VideoObjectPlaneH263.num_gobs_in_vop = 0;//***???
    VOL->VideoObjectPlaneH263.num_macroblocks_in_gob = 0;//***mp4_H263_mbgob[i];
    VOL->vop_time_increment_resolution = 30000;
        // VOL->VideoObjectPlane.time_increment = -1001;

    return MP4_STATUS_OK;
}

#ifndef SUPPORT_H263_ONLY 
static mp4_Status mp4_Sprite_Trajectory(mp4_Info* pInfo) {
    int     i, dmv_code, dmv_length, fb;
    Ipp32u  code;

    for (i = 0; i < pInfo->VisualObject.VideoObject.sprite_warping_points; i ++) {
        code = mp4_ShowBits9(pInfo, 3);
        if (code == 7) {
            mp4_FlushBits(pInfo, 3);
            code = mp4_ShowBits9(pInfo, 9);
            fb = 1;
            while (code & 256) {
                code <<= 1;
                fb ++;
            }
            if (fb > 9) {
                mp4_Error("Error when decode sprite_trajectory");
                return MP4_STATUS_PARSE_ERROR;
            }
            dmv_length = fb + 5;
        } else {
            fb = (code <= 1) ? 2 : 3;
            dmv_length = code - 1;
        }
        mp4_FlushBits(pInfo, fb);
        if (dmv_length <= 0)
            dmv_code = 0;
        else {
            dmv_code = mp4_GetBits(pInfo, dmv_length);
            if ((dmv_code & (1 << (dmv_length - 1))) == 0)
                dmv_code -= (1 << dmv_length) - 1;
        }
        if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
        pInfo->VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_du[i] = dmv_code;
        code = mp4_ShowBits9(pInfo, 3);
        if (code == 7) {
            mp4_FlushBits(pInfo, 3);
            code = mp4_ShowBits9(pInfo, 9);
            fb = 1;
            while (code & 256) {
                code <<= 1;
                fb ++;
            }
            if (fb > 9) {
                mp4_Error("Error when decode sprite_trajectory");
                return MP4_STATUS_PARSE_ERROR;
            }
            dmv_length = fb + 5;
        } else {
            fb = (code <= 1) ? 2 : 3;
            dmv_length = code - 1;
        }
        mp4_FlushBits(pInfo, fb);
        if (dmv_length <= 0)
            dmv_code = 0;
        else {
            dmv_code = mp4_GetBits(pInfo, dmv_length);
            if ((dmv_code & (1 << (dmv_length - 1))) == 0)
                dmv_code -= (1 << dmv_length) - 1;
        }
        if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
        pInfo->VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_dv[i] = dmv_code;
    }
    return MP4_STATUS_OK;
}

mp4_Status mp4_Parse_GroupOfVideoObjectPlane(mp4_Info* pInfo)
{
    Ipp32u  hour, min, sec;

    hour = mp4_GetBits9(pInfo, 5);
    min = mp4_GetBits9(pInfo, 6);
    if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
    sec = mp4_GetBits9(pInfo, 6);
    pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane.time_code = sec + min * 60 + hour * 3600;
    pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane.time_code *= pInfo->VisualObject.VideoObject.vop_time_increment_resolution;
    pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane.closed_gov = mp4_GetBit(pInfo);
    pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane.broken_link = mp4_GetBit(pInfo);
    return MP4_STATUS_OK;
}
#endif

mp4_Status mp4_Parse_VideoObjectPlane(mp4_Info* pInfo)
{
    Ipp32u  code;
    mp4_VideoObject          *VOL = &pInfo->VisualObject.VideoObject;
    mp4_VideoObjectPlane     *VOP = &pInfo->VisualObject.VideoObject.VideoObjectPlane;
    mp4_VideoObjectPlaneH263 *VOPSH = &pInfo->VisualObject.VideoObject.VideoObjectPlaneH263;

	//***
	VOP->droppable = 0; 

    if (VOL->short_video_header) 
	{
        if( VOL->h263_flv <=1 )
		{

			if( VOL->h263_plus )
			{		
				code = mp4_GetBits9(pInfo, 6); //make sure first 22 bits are skipped
				return mp4_Parse_VideoObject_h263_plus(pInfo);
			}
			else
			{
				code = mp4_GetBits9(pInfo, 6); // read rest bits of short_video_start_marker
				VOPSH->temporal_reference = mp4_GetBits9(pInfo, 8);
				if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
				code = mp4_GetBit(pInfo); // zero_bit
				VOPSH->split_screen_indicator = mp4_GetBit(pInfo);
				VOPSH->document_camera_indicator = mp4_GetBit(pInfo);
				VOPSH->full_picture_freeze_release = mp4_GetBit(pInfo);
				VOPSH->source_format = mp4_GetBits9(pInfo, 3);
				if (VOPSH->source_format == 0 || VOPSH->source_format > 5) {
					mp4_Error("Error: Bad value for VideoPlaneWithShortHeader.source_format");
					return MP4_STATUS_PARSE_ERROR;
				}
				VOPSH->picture_coding_type = mp4_GetBit(pInfo);
				VOP->coding_type = VOPSH->picture_coding_type;
				code = mp4_GetBits9(pInfo, 4); // four_reserved_zero_bits
				VOPSH->vop_quant = mp4_GetBits9(pInfo, 5);
				code = mp4_GetBit(pInfo); // zero_bit
				for (;;) {
					code = mp4_GetBit(pInfo); // pei
					if (!code)
						break;
					code = mp4_GetBits9(pInfo, 8); // psupp
				}
				return MP4_STATUS_OK;
			}
		}
		else
		{
			int deblockon;
			int width;
			int height;

			code = mp4_GetBits9(pInfo, 6); // read rest bits of short_video_start_marker
			VOPSH->temporal_reference = mp4_GetBits9(pInfo, 8);
			VOPSH->source_format = mp4_GetBits9(pInfo, 3);
			if( VOPSH->source_format == 0 )
			{
				width  = mp4_GetBits9(pInfo, 8);
				height = mp4_GetBits9(pInfo, 8);
			}
			else if( VOPSH->source_format == 1 )
			{
				width  = mp4_GetBits(pInfo, 16);
				height = mp4_GetBits(pInfo, 16);
			}
			else if(  VOPSH->source_format == 2 )
			{
				width = 352;
				height  = 288;
			}
			else if(  VOPSH->source_format == 3 )
			{
				width = 176;
				height  = 144;
			}
			else if(  VOPSH->source_format == 4 )
			{
				width = 128;
				height  = 96;
			}
			else if(  VOPSH->source_format == 5 )
			{
				width = 320;
				height  = 240;
			}
			else if(  VOPSH->source_format == 6 )
			{
				width = 160;
				height  = 120;
			}
			else
			{
				width = 0;
				height  = 0;
			}

			VOPSH->picture_coding_type = mp4_GetBits9(pInfo, 2);
			if( VOPSH->picture_coding_type > 1 )
			{
				VOPSH->picture_coding_type = 1;
				VOP->droppable = 1; 
			}

			VOP->coding_type = VOPSH->picture_coding_type;
			deblockon = mp4_GetBit(pInfo); // pei
			VOPSH->vop_quant = mp4_GetBits9(pInfo, 5);
			for (;;) 
			{
				code = mp4_GetBit(pInfo); // pei
				if (!code)
					break;
				code = mp4_GetBits9(pInfo, 8); // psupp
			}
			return MP4_STATUS_OK;
		}
    }
    
#ifdef SUPPORT_H263_ONLY 
	return MP4_STATUS_FILE_ERROR;
#else
    
    VOP->coding_type = mp4_GetBits9(pInfo, 2);
    VOP->modulo_time_base = 0;
    do {
        code = mp4_GetBit(pInfo);
        VOP->modulo_time_base += code;
    } while (code);
    if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
    if (VOL->vop_time_increment_resolution_bits != 0 ) {
        VOP->time_increment = mp4_GetBits(pInfo, VOL->vop_time_increment_resolution_bits);
    }
    if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
    VOP->coded = mp4_GetBit(pInfo);
    if (!VOP->coded)
        return MP4_STATUS_OK;
    //f if (newpred_enable)
    if (VOL->shape != MP4_SHAPE_TYPE_BINARYONLY && (VOP->coding_type == MP4_VOP_TYPE_P ||
        (VOP->coding_type == MP4_VOP_TYPE_S && VOL->sprite_enable == MP4_SPRITE_GMC)))
        VOP->rounding_type = mp4_GetBit(pInfo);
    if (VOL->reduced_resolution_vop_enable && VOL->shape == MP4_SHAPE_TYPE_RECTANGULAR &&
        (VOP->coding_type == MP4_VOP_TYPE_I || VOP->coding_type == MP4_VOP_TYPE_P)) {
        VOP->reduced_resolution = mp4_GetBit(pInfo);
        if (VOP->reduced_resolution) {
            mp4_Error("Error: Reduced Resolution VOP is not supported");
            return MP4_STATUS_PARSE_ERROR;
        }
    }
    if (VOL->shape != MP4_SHAPE_TYPE_RECTANGULAR) {
        if (!(VOL->sprite_enable == MP4_SPRITE_STATIC && VOP->coding_type == MP4_VOP_TYPE_I)) {
            VOP->vop_width = mp4_GetBits(pInfo, 13);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOP->vop_height = mp4_GetBits(pInfo, 13);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOP->vop_horizontal_mc_spatial_ref = mp4_GetBits(pInfo, 13);
            VOP->vop_horizontal_mc_spatial_ref <<= (32 - 13);
            VOP->vop_horizontal_mc_spatial_ref >>= (32 - 13);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
            VOP->vop_vertical_mc_spatial_ref = mp4_GetBits(pInfo, 13);
            VOP->vop_vertical_mc_spatial_ref <<= (32 - 13);
            VOP->vop_vertical_mc_spatial_ref >>= (32 - 13);
            if (!mp4_GetMarkerBit(pInfo)) return MP4_STATUS_PARSE_ERROR;
        }
//f        if ((VOL->shape != MP4_SHAPE_TYPE_BINARYONLY) && VOL->scalability && enhancement_type)
//f            background_composition = mp4_GetBit(pInfo);
        VOP->change_conv_ratio_disable = mp4_GetBit(pInfo);
        VOP->vop_constant_alpha = mp4_GetBit(pInfo);
        if (VOP->vop_constant_alpha)
            VOP->vop_constant_alpha_value = mp4_GetBits9(pInfo, 8);
        else
            VOP->vop_constant_alpha_value = 255;
    }
    if (VOL->shape != MP4_SHAPE_TYPE_BINARYONLY) {
        if (!VOL->complexity_estimation_disable) {
            if (VOL->ComplexityEstimation.estimation_method == 0) {
                if (VOP->coding_type == MP4_VOP_TYPE_I) {
                    if (VOL->ComplexityEstimation.opaque) VOL->ComplexityEstimation.dcecs_opaque =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.transparent) VOL->ComplexityEstimation.dcecs_transparent =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.intra_cae) VOL->ComplexityEstimation.dcecs_intra_cae =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.inter_cae) VOL->ComplexityEstimation.dcecs_inter_cae =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.no_update) VOL->ComplexityEstimation.dcecs_no_update =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.upsampling) VOL->ComplexityEstimation.dcecs_upsampling =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.intra_blocks) VOL->ComplexityEstimation.dcecs_intra_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.not_coded_blocks) VOL->ComplexityEstimation.dcecs_not_coded_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_coefs) VOL->ComplexityEstimation.dcecs_dct_coefs =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_lines) VOL->ComplexityEstimation.dcecs_dct_lines =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_symbols) VOL->ComplexityEstimation.dcecs_vlc_symbols =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_bits) VOL->ComplexityEstimation.dcecs_vlc_bits =  mp4_GetBits9(pInfo, 4);
                    if (VOL->ComplexityEstimation.sadct) VOL->ComplexityEstimation.dcecs_sadct =  mp4_GetBits9(pInfo, 8);
                }
                if (VOP->coding_type == MP4_VOP_TYPE_P) {
                    if (VOL->ComplexityEstimation.opaque) VOL->ComplexityEstimation.dcecs_opaque =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.transparent) VOL->ComplexityEstimation.dcecs_transparent =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.intra_cae) VOL->ComplexityEstimation.dcecs_intra_cae =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.inter_cae) VOL->ComplexityEstimation.dcecs_inter_cae =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.no_update) VOL->ComplexityEstimation.dcecs_no_update =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.upsampling) VOL->ComplexityEstimation.dcecs_upsampling =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.intra_blocks) VOL->ComplexityEstimation.dcecs_intra_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.not_coded_blocks) VOL->ComplexityEstimation.dcecs_not_coded_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_coefs) VOL->ComplexityEstimation.dcecs_dct_coefs =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_lines) VOL->ComplexityEstimation.dcecs_dct_lines =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_symbols) VOL->ComplexityEstimation.dcecs_vlc_symbols =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_bits) VOL->ComplexityEstimation.dcecs_vlc_bits =  mp4_GetBits9(pInfo, 4);
                    if (VOL->ComplexityEstimation.inter_blocks) VOL->ComplexityEstimation.dcecs_inter_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.inter4v_blocks) VOL->ComplexityEstimation.dcecs_inter4v_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.apm) VOL->ComplexityEstimation.dcecs_apm =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.npm) VOL->ComplexityEstimation.dcecs_npm =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.forw_back_mc_q) VOL->ComplexityEstimation.dcecs_forw_back_mc_q =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.halfpel2) VOL->ComplexityEstimation.dcecs_halfpel2 =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.halfpel4) VOL->ComplexityEstimation.dcecs_halfpel4 =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.sadct) VOL->ComplexityEstimation.dcecs_sadct =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.quarterpel) VOL->ComplexityEstimation.dcecs_quarterpel =  mp4_GetBits9(pInfo, 8);
                }
                if (VOP->coding_type == MP4_VOP_TYPE_B) {
                    if (VOL->ComplexityEstimation.opaque) VOL->ComplexityEstimation.dcecs_opaque =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.transparent) VOL->ComplexityEstimation.dcecs_transparent =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.intra_cae) VOL->ComplexityEstimation.dcecs_intra_cae =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.inter_cae) VOL->ComplexityEstimation.dcecs_inter_cae =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.no_update) VOL->ComplexityEstimation.dcecs_no_update =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.upsampling) VOL->ComplexityEstimation.dcecs_upsampling =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.intra_blocks) VOL->ComplexityEstimation.dcecs_intra_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.not_coded_blocks) VOL->ComplexityEstimation.dcecs_not_coded_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_coefs) VOL->ComplexityEstimation.dcecs_dct_coefs =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_lines) VOL->ComplexityEstimation.dcecs_dct_lines =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_symbols) VOL->ComplexityEstimation.dcecs_vlc_symbols =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_bits) VOL->ComplexityEstimation.dcecs_vlc_bits =  mp4_GetBits9(pInfo, 4);
                    if (VOL->ComplexityEstimation.inter_blocks) VOL->ComplexityEstimation.dcecs_inter_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.inter4v_blocks) VOL->ComplexityEstimation.dcecs_inter4v_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.apm) VOL->ComplexityEstimation.dcecs_apm =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.npm) VOL->ComplexityEstimation.dcecs_npm =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.forw_back_mc_q) VOL->ComplexityEstimation.dcecs_forw_back_mc_q =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.halfpel2) VOL->ComplexityEstimation.dcecs_halfpel2 =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.halfpel4) VOL->ComplexityEstimation.dcecs_halfpel4 =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.interpolate_mc_q) VOL->ComplexityEstimation.dcecs_interpolate_mc_q =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.sadct) VOL->ComplexityEstimation.dcecs_sadct =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.quarterpel) VOL->ComplexityEstimation.dcecs_quarterpel =  mp4_GetBits9(pInfo, 8);
                }
                if (VOP->coding_type == MP4_VOP_TYPE_S && VOL->sprite_enable == MP4_SPRITE_STATIC) {
                    if (VOL->ComplexityEstimation.intra_blocks) VOL->ComplexityEstimation.dcecs_intra_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.not_coded_blocks) VOL->ComplexityEstimation.dcecs_not_coded_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_coefs) VOL->ComplexityEstimation.dcecs_dct_coefs =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.dct_lines) VOL->ComplexityEstimation.dcecs_dct_lines =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_symbols) VOL->ComplexityEstimation.dcecs_vlc_symbols =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.vlc_bits) VOL->ComplexityEstimation.dcecs_vlc_bits =  mp4_GetBits9(pInfo, 4);
                    if (VOL->ComplexityEstimation.inter_blocks) VOL->ComplexityEstimation.dcecs_inter_blocks =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.inter4v_blocks) VOL->ComplexityEstimation.dcecs_inter4v_blocks =  mp4_GetBits(pInfo, 8);
                    if (VOL->ComplexityEstimation.apm) VOL->ComplexityEstimation.dcecs_apm =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.npm) VOL->ComplexityEstimation.dcecs_npm =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.forw_back_mc_q) VOL->ComplexityEstimation.dcecs_forw_back_mc_q =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.halfpel2) VOL->ComplexityEstimation.dcecs_halfpel2 =  mp4_GetBits9(pInfo, 8);
                    if (VOL->ComplexityEstimation.halfpel4) VOL->ComplexityEstimation.dcecs_halfpel4 =  mp4_GetBits(pInfo, 8);
                    if (VOL->ComplexityEstimation.interpolate_mc_q) VOL->ComplexityEstimation.dcecs_interpolate_mc_q =  mp4_GetBits9(pInfo, 8);
                }
            }
        }
        VOP->intra_dc_vlc_thr = mp4_GetBits9(pInfo, 3);
        if (VOL->interlaced) {
            VOP->top_field_first = mp4_GetBit(pInfo);
            VOP->alternate_vertical_scan_flag = mp4_GetBit(pInfo);
        }
    }
    if ((VOL->sprite_enable == MP4_SPRITE_STATIC || VOL->sprite_enable == MP4_SPRITE_GMC) && VOP->coding_type == MP4_VOP_TYPE_S) {
        if (VOL->sprite_warping_points > 0)
            if (mp4_Sprite_Trajectory(pInfo) != MP4_STATUS_OK)
                return MP4_STATUS_PARSE_ERROR;
        if (VOL->sprite_brightness_change) {
            code = mp4_ShowBits9(pInfo, 4);
            if (code == 15) {
                mp4_FlushBits(pInfo, 4);
                VOP->brightness_change_factor = 625 + mp4_GetBits(pInfo, 10);
            } else if (code == 14) {
                mp4_FlushBits(pInfo, 4);
                VOP->brightness_change_factor = 113 + mp4_GetBits9(pInfo, 9);
            } else if (code >= 12) {
                mp4_FlushBits(pInfo, 3);
                code = mp4_GetBits9(pInfo, 7);
                VOP->brightness_change_factor = (code < 64) ? code - 112 : code - 15;
            } else if (code >= 8) {
                mp4_FlushBits(pInfo, 2);
                code = mp4_GetBits9(pInfo, 6);
                VOP->brightness_change_factor = (code < 32) ? code - 48 : code - 15;
            } else {
                mp4_FlushBits(pInfo, 1);
                code = mp4_GetBits9(pInfo, 5);
                VOP->brightness_change_factor = (code < 16) ? code - 16 : code - 15;
            }
        } else
            VOP->brightness_change_factor = 0;
        if (VOL->sprite_enable == MP4_SPRITE_STATIC)
            return MP4_STATUS_OK;
    }
    if (VOL->shape != MP4_SHAPE_TYPE_BINARYONLY) {
        VOP->quant = mp4_GetBits9(pInfo, VOL->quant_precision);
        if (VOL->shape == MP4_SHAPE_TYPE_GRAYSCALE) {
            int   ac, i;

            ac = mp4_aux_comp_count[VOL->shape_extension];
            for (i = 0; i < ac; i ++)
                VOP->alpha_quant[i] = mp4_GetBits9(pInfo, 6);
        }
        if (VOP->coding_type != MP4_VOP_TYPE_I) {
            VOP->fcode_forward = mp4_GetBits9(pInfo, 3);
            if (VOP->fcode_forward == 0) {
                mp4_Error("Error: vop_fcode_forward == 0");
                return MP4_STATUS_PARSE_ERROR;
            }
        }
        if (VOP->coding_type == MP4_VOP_TYPE_B) {
            VOP->fcode_backward = mp4_GetBits9(pInfo, 3);
            if (VOP->fcode_backward == 0) {
                mp4_Error("Error: vop_fcode_backward == 0");
                return MP4_STATUS_PARSE_ERROR;
            }
        }
        if (!VOL->scalability) {
            if (VOL->shape != MP4_SHAPE_TYPE_RECTANGULAR && VOP->coding_type != MP4_VOP_TYPE_I)
                VOP->shape_coding_type = mp4_GetBit(pInfo);
        } else {
            //f if (VOL->enhancement_type) {
                //f VOP->load_backward_shape = mp4_GetBit(pInfo);
                //f if (VOP->load_backward_shape) {
                    //f shape
                //f }
            //f }
            VOP->ref_select_code = mp4_GetBits9(pInfo, 2);
        }
    }
    return MP4_STATUS_OK;
    
 #endif   
}


#ifndef SUPPORT_H263_ONLY 
/*
// decode VideoPacket
*/
mp4_Status mp4_DecodeVideoPacket(mp4_Info* pInfo, int *quant_scale, int *found)
{
    Ipp32u      code;
    int         header_extension_code, rml;
    mp4_VideoObject       *VOL = &pInfo->VisualObject.VideoObject;
    mp4_VideoObjectPlane  *VOP = &pInfo->VisualObject.VideoObject.VideoObjectPlane;

    if (VOP->coding_type == MP4_VOP_TYPE_I)
        rml = 17;
    else if (VOP->coding_type == MP4_VOP_TYPE_B)
        rml = 16 + IPP_MAX(VOP->fcode_forward, VOP->fcode_backward);
    else
        rml = 16 + VOP->fcode_forward;
    if (mp4_CheckResyncMarker(pInfo, rml)) { // check resync_marker
        mp4_AlignBits7F(pInfo);
        code = mp4_GetBits(pInfo, rml);
        header_extension_code = 0;
        if (VOL->shape != MP4_SHAPE_TYPE_RECTANGULAR) {
            header_extension_code = mp4_GetBit(pInfo);
            if (header_extension_code && !(VOL->sprite_enable == MP4_SPRITE_STATIC && VOP->coding_type == MP4_VOP_TYPE_I)) {
                VOP->vop_width = mp4_GetBits(pInfo, 13);
                if (!mp4_GetBit(pInfo)) goto err;
                VOP->vop_height = mp4_GetBits(pInfo, 13);
                if (!mp4_GetBit(pInfo))  goto err;
                VOP->vop_horizontal_mc_spatial_ref = mp4_GetBits(pInfo, 13);
                if (!mp4_GetBit(pInfo)) goto err;
                VOP->vop_vertical_mc_spatial_ref = mp4_GetBits(pInfo, 13);
                if (!mp4_GetBit(pInfo))  goto err;
            }
        }
        //f ignore macroblock_number. For error resilience we need use it
        code = mp4_GetBits(pInfo, pInfo->VisualObject.VideoObject.mbns);
        if (VOL->shape != MP4_SHAPE_TYPE_BINARYONLY) {
            *quant_scale = mp4_GetBits9(pInfo, VOL->quant_precision); // quant_scale
        }
        if (VOL->shape == MP4_SHAPE_TYPE_RECTANGULAR) {
            header_extension_code = mp4_GetBit(pInfo);
        }
        if (header_extension_code) {
            //f ignore modulo_time_base
            do {
                code = mp4_GetBit(pInfo);
            } while (code);
            if (!mp4_GetBit(pInfo)) goto err;
            //f ignore vop_time_increment
            if (VOL->vop_time_increment_resolution_bits != 0 ) {
                code = mp4_GetBits(pInfo, VOL->vop_time_increment_resolution_bits);
            }
            if (!mp4_GetBit(pInfo)) goto err;
            //f ignore vop_coding_type
            code = mp4_GetBits9(pInfo, 2);
            if (VOL->shape != MP4_SHAPE_TYPE_RECTANGULAR) {
                VOP->change_conv_ratio_disable = mp4_GetBit(pInfo);
                if (VOP->coding_type != MP4_VOP_TYPE_I)
                    VOP->shape_coding_type = mp4_GetBit(pInfo);
            }
            if (VOL->shape != MP4_SHAPE_TYPE_BINARYONLY) {
                //f ignore intra_dc_vlc_thr
                code = mp4_GetBits9(pInfo, 3);
                if (VOL->sprite_enable == MP4_SPRITE_GMC && VOP->coding_type == MP4_VOP_TYPE_S && VOL->sprite_warping_points > 0)
                    if (mp4_Sprite_Trajectory(pInfo) != MP4_STATUS_OK)
                        goto err;
                //f ignore vop_reduced_resolution
                if (VOL->reduced_resolution_vop_enable && VOL->shape == MP4_SHAPE_TYPE_RECTANGULAR &&
                    (VOP->coding_type == MP4_VOP_TYPE_I || VOP->coding_type == MP4_VOP_TYPE_P))
                    code = mp4_GetBit(pInfo);
                if (VOP->coding_type != MP4_VOP_TYPE_I)
                    VOP->fcode_forward = mp4_GetBits9(pInfo, 3);
                if (VOP->coding_type == MP4_VOP_TYPE_B)
                    VOP->fcode_backward = mp4_GetBits9(pInfo, 3);
            }
        }
        if (VOL->newpred_enable) {
        }
        *found = 1;
        return MP4_STATUS_OK;
    }
    *found = 0;
    return MP4_STATUS_OK;
err:
    mp4_Error("Error when decode Video Packet Header");
    return MP4_STATUS_PARSE_ERROR;
}



#endif
