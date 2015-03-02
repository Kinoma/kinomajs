/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SAACDEC_TNS_H
#define __SAACDEC_TNS_H

#include "aaccmn_const.h"
#include "aac_dec_own.h"

typedef struct
{
    Ipp32s   m_lpc[MAX_NUM_WINDOWS][MAX_FILT][TNS_MAX_ORDER];
    int      m_lpc_scalef[MAX_NUM_WINDOWS][MAX_FILT];
    int      m_start[MAX_NUM_WINDOWS][MAX_FILT];
    int      m_size[MAX_NUM_WINDOWS][MAX_FILT];
    int      m_order[MAX_NUM_WINDOWS][MAX_FILT];
    int      m_inc[MAX_NUM_WINDOWS][MAX_FILT];

    int      m_num_windows;
    int      m_n_filt[MAX_NUM_WINDOWS];
    int      m_tns_data_present;
} s_tns_data;

#ifdef  __cplusplus
extern "C" {
#endif

void ics_calc_tns_data(s_SE_Individual_channel_stream *p_stream, s_tns_data *p_data);
void ics_apply_tns_enc_I(s_tns_data* p_data, Ipp32s *p_spectrum);
void ics_apply_tns_dec_I(s_tns_data* p_data, Ipp32s *p_spectrum);

#ifdef  __cplusplus
}
#endif


#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef PI
#define PI 3.14159265359f
#endif


#endif//__SAACDEC_LTP_H
