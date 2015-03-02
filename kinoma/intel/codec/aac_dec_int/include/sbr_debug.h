/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SBR_DEBUG_H__
#define __SBR_DEBUG_H__

#ifdef _DEBUG

#include <stdio.h>

#define NAME_FILE_LOG "he_aac.log"

extern FILE* file_info;

#ifdef  __cplusplus
extern "C" {
#endif

int initDbgInfo( void);
int FreeDbgInfo(void);

#ifdef  __cplusplus
}
#endif

#endif

#endif /* __SBR_DEBUG_H__ */
/* EOF */

