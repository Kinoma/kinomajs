/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3DEC_HUFTABS_H__
#define __MP3DEC_HUFTABS_H__

#include "ippdc.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int mp3dec_VLCShifts[];
extern int mp3dec_VLCOffsets[];
extern int mp3dec_VLCTypes[];
extern int mp3dec_VLCTableSizes[];
extern int mp3dec_VLCNumSubTables[];
extern int *mp3dec_VLCSubTablesSizes[];
extern IppsVLCTable_32s *mp3dec_VLCBooks[];

#ifdef __cplusplus
}
#endif

#endif //__HUFTABS_H__
