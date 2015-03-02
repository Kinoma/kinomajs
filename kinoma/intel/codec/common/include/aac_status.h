/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __AAC_STATUS_H__
#define __AAC_STATUS_H__

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum
  {
      AAC_OK = 0,
      AAC_NOT_ENOUGH_DATA,
      AAC_BAD_FORMAT,
      AAC_ALLOC,
      AAC_BAD_STREAM,
      AAC_NULL_PTR,
      AAC_NOT_FIND_SYNCWORD,
      AAC_BAD_PARAMETER,
      AAC_UNSUPPORTED
  } AACStatus;

#ifdef __cplusplus
}
#endif

#endif
