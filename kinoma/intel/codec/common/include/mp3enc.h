/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "ipps.h"
#include "audio_codec_params.h"

#ifndef __MP3ENC_H__
#define __MP3ENC_H__

#include "mp3_status.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MP3Enc;
typedef struct _MP3Enc MP3Enc;

MP3Status mp3encInit(MP3Enc **state_ptr,
                     int sampling_frequency,
                     int stereo,
                     int bitrate);
MP3Status mp3encClose(MP3Enc *state);
MP3Status mp3encGetFrame(Ipp16s *inPointer, int *encodedBytes,
                         Ipp8u *outPointer, MP3Enc *state);
int mp3enc_checkBitRate(int br, int *ind);

#ifdef __cplusplus
}
#endif

#endif
