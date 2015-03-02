/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SBRDEC_SETTING_INT_H__
#define __SBRDEC_SETTING_INT_H__

#define SCALE_FACTOR_QMFA_IN        14

#define SCALE_FACTOR_QMFA_OUT        5

#define SCALE_FACTOR_LP_COEFS       29

#define SCALE_FACTOR_NOISE_DEQUANT  24

#define THRESHOLD_LP_COEFS          0X40000000

#define THRESHOLD_GAIN              0XDEAD // it is MARKER only (pointer to 10^10)

#define SCALE_FACTOR_SUM64          6

#define SCALE_FACTOR_G_LIM_BOOST   24

#define SCALE_FACTOR_QM_LIM_BOOST  14

#define SCALE_FACTOR_SM_BOOST      SCALE_FACTOR_QMFA_OUT

#endif //__SBRDEC_SETTING_INT_H__
