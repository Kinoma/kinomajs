/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3_STATUS_H__
#define __MP3_STATUS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MP3_OK = 0,
    MP3_NOT_ENOUGH_DATA,
    MP3_BAD_FORMAT,
    MP3_ALLOC,
    MP3_BAD_STREAM,
    MP3_NULL_PTR,
    MP3_NOT_FIND_SYNCWORD,
    MP3_NOT_ENOUGH_BUFFER,
    MP3_FAILED_TO_INITIALIZE,
    MP3_UNSUPPORTED
} MP3Status;

#ifdef __cplusplus
}
#endif

#endif
