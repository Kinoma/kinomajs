/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __AAC_DEC_H__
#define __AAC_DEC_H__

#include "ipps.h"
#include "aaccmn_const.h"
#include "audio_codec_params.h"
#include "mp4cmn_pce.h"
#include "aac_status.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct _AACDec;
  typedef struct _AACDec AACDec;

  //***
  void	aacdecSetIgnoreFIL(AACDec *state);

  AACStatus aacdecReset(AACDec *state);
  AACStatus aacdecInit(AACDec **state_ptr);
  AACStatus aacdecClose(AACDec *state);
  AACStatus aacdecGetFrame(Ipp8u *inPointer,
                           int *decodedBytes,
                           Ipp16s *outPointer,
                           AACDec *state);
  AACStatus aacdecGetDuration(float *p_duration,
                            AACDec *state);
  AACStatus aacdecGetInfo(cAudioCodecParams *a_info,
                          AACDec *state);

  AACStatus aacdecSetSamplingFrequency(int sampling_frequency_index,
                                       AACDec *state);
  AACStatus aacdecSetSBRModeDecode(int ModeDecodeHEAAC,
                                   AACDec *state);
  AACStatus aacdecSetSBRModeDwnmx(int ModeDwnsmplHEAAC,
                                  AACDec *state);
  AACStatus aacdecSetAudioObjectType(enum AudioObjectType audio_object_type,
                                     AACDec *state);
  AACStatus aacdecSetPCE(sProgram_config_element *pce,
                         AACDec *state);
  AACStatus aacdecSetAdtsChannelConfiguration(int adts_channel_configuration,
                                              AACDec *state);
  AACStatus aacdecGetSbrFlagPresent(int *SbrFlagPresent,
                                    AACDec *state);
  AACStatus aacdecGetFrameSize(int *frameSize,
                               AACDec *state);
  AACStatus aacdecGetSampleFrequency(int *freq,
                                     AACDec *state);
  AACStatus aacdecGetChannels(int *ch,
                              AACDec *state);

  AACStatus aacdec_GetID3Len(Ipp8u *in,
                             int inDataSize,
                             AACDec *state);

  AACStatus aacdec_SkipID3(int inDataSize,
                           int *skipped,
                           AACDec *state);
#ifdef __cplusplus
}
#endif

#endif
