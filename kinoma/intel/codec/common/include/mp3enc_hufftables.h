/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3ENC_HUFFTABLES_H__
#define __MP3ENC_HUFFTABLES_H__

#include "ippdc.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int mp3enc_VLCShifts[];
extern int mp3enc_VLCOffsets[];
extern int mp3enc_VLCTypes[];
extern int mp3enc_VLCTableSizes[];
extern int mp3enc_VLCNumSubTables[];
extern int *mp3enc_VLCSubTablesSizes[];
extern IppsVLCTable_32s *mp3enc_VLCBooks[];

extern short mp3enc_table32[];
extern short mp3enc_table33[];

#ifdef __cplusplus
}
#endif

#endif  //      __MP3ENC_HUFFTABLES_H__
