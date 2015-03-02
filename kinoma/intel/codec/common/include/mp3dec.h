/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3DEC_H__
#define __MP3DEC_H__

#include "ipps.h"
#include "audio_codec_params.h"
#include "mp3_status.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MP3Dec;
typedef struct _MP3Dec MP3Dec;

MP3Status mp3decInit(MP3Dec **state_ptr);
MP3Status mp3decClose(MP3Dec *state);
MP3Status mp3decGetInfo(cAudioCodecParams *a_info, MP3Dec *state);
MP3Status mp3decGetDuration(float *p_duration, MP3Dec *state);
MP3Status mp3decReset(MP3Dec *state);
MP3Status mp3decGetFrame(Ipp8u *inPointer, int inDataSize, int *decodedBytes,
                         Ipp16s *outPointer, int outBufferSize, MP3Dec *state);
MP3Status mp3decGetSampleFrequency(int *freq, MP3Dec *state);
MP3Status mp3decGetFrameSize(int *frameSize, MP3Dec *state);
MP3Status mp3decGetChannels(int *ch, MP3Dec *state);

#ifdef __cplusplus
}
#endif

#endif
