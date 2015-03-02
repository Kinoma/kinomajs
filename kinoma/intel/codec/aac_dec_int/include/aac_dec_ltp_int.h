/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SAACDEC_LTP_H
#define __SAACDEC_LTP_H

#include "aac_filterbank_int.h"
#include "aac_dec_tns_int.h"
#include "aac_dec_own.h"

typedef struct
{

    Ipp16s* p_samples_1st_part;
    Ipp16s* p_samples_2nd_part;
    Ipp16s* p_samples_3rd_part;
    int    prev_windows_shape;

    sFilterbankInt* p_filterbank_data;
    s_tns_data*  p_tns_data;

} sLtp;

#ifdef  __cplusplus
extern "C" {
#endif

void ics_apply_ltp_I(sLtp* p_data,s_SE_Individual_channel_stream * p_stream, Ipp32s * p_spectrum);

#ifdef  __cplusplus
}
#endif

#endif//__SAACDEC_LTP_H
