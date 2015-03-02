/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives AAC Decode Sample for Windows*
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//  MPEG-4 and AAC are international standards promoted by ISO, IEC, ITU, ETSI
//  and other organizations. Implementations of these standards, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
*/
#include "ippdc.h"
#ifndef __HUFF_TABLES_H
#define __HUFF_TABLES_H

#ifdef  __cplusplus
extern "C" {
#endif

extern int vlcShifts[];
extern int vlcOffsets[];
extern int vlcTypes[];
extern int vlcTableSizes[];
extern int vlcNumSubTables[];
extern int *vlcSubTablesSizes[];
extern IppsVLCTable_32s *vlcBooks[];

#ifdef  __cplusplus
}
#endif


#endif//__HUFF_TABLES_H
