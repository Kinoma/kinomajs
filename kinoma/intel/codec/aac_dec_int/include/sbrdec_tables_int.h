/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#include "ippdefs.h"
#include "ippdc.h"
#include "sbrdec_huftabs.h"

#ifndef __SBRDEC_TABLES_INT_H__
#define __SBRDEC_TABLES_INT_H__

#ifdef  __cplusplus
extern "C" {
#endif

/* QMF WINDOW */
extern Ipp32s SBR_TABLE_QMF_WINDOW_320_INT_Q30[];
extern Ipp32s SBR_TABLE_QMF_WINDOW_320_INT_Q31[];
extern Ipp32s SBR_TABLE_QMF_WINDOW_640_INT_Q31[];

#if 0
extern Ipp32f SBR_TABLE_QMF_WINDOW_640_FP[];
extern Ipp32f SBR_TABLE_QMF_WINDOW_320_FP[];
#endif

/* ANALYSIS QMF */
extern Ipp32s SBR_TABLE_PRE_QMFA_COS_INT_Q30[];
extern Ipp32s SBR_TABLE_PRE_QMFA_SIN_INT_Q30[];

extern Ipp32s SBR_TABLE_POST_QMFA_COS_INT_Q30[];
extern Ipp32s SBR_TABLE_POST_QMFA_SIN_INT_Q30[];

/* SYNTHESIS QMF */
extern Ipp32s SBR_TABLE_PRE_QMFS_COS_INT_Q31[];
extern Ipp32s SBR_TABLE_PRE_QMFS_SIN_INT_Q31[];

extern Ipp32s SBR_TABLE_POST_QMFS_COS_INT_Q31[];
extern Ipp32s SBR_TABLE_POST_QMFS_SIN_INT_Q31[];

/* SYNTHESIS DOWN QMF */
extern Ipp32s SBR_TABLE_PRE_QMFSD_COS_INT_Q31[];
extern Ipp32s SBR_TABLE_PRE_QMFSD_SIN_INT_Q31[];

extern Ipp32s SBR_TABLE_POST_QMFSD_COS_INT_Q31[];
extern Ipp32s SBR_TABLE_POST_QMFSD_SIN_INT_Q31[];

/* NOISE TABLE is used by sbrAdjustmentHF */
extern int* SBR_TABLE_V_INT_Q30[2];

extern const int SBR_TABLE_INVERT[];

#ifdef  __cplusplus
}
#endif

#endif/*__SBRDEC_TABLES_INT_H__ */
/* EOF */
