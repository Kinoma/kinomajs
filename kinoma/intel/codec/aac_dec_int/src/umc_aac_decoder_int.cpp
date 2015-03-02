/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "umc_aac_decoder_int.h"
#include "mp4cmn_config.h"

namespace UMC {

/********************************************************************/

  AACDecoderInt::AACDecoderInt()
  {
    state = NULL;
  }

/********************************************************************/

  Status AACDecoderInt::Init(BaseCodecParams_V51 * init)
  {
    AudioCodecParams_V51 *pAudioCodecInit = (AudioCodecParams_V51 *) init;
    //AACDecoderParams* pAACDecoderParams;

    if (!pAudioCodecInit)
      return UMC_NULL_PTR;

    aacdecInit(&state);
   /* ***************************** *
    * usr settings for HEAAC
    * status = SetParams(init) - incorrect here
    * because default settings apply by constructor()
    * ***************************** */
    SetParams(init) ;

    initSubtype = 0;
    m_info.m_audio_object_type = AOT_UNDEF;
    m_info.m_decode_mode = DM_UNDEF_STREAM;
    m_info.m_init_stream_type = pAudioCodecInit->m_info_in.stream_type;
    if (NULL == pAudioCodecInit->m_pData) {
      m_info.m_init_config_data_size = 0;
    } else {
      m_info.m_init_config_data_size = pAudioCodecInit->m_pData->GetDataSize();
    }
    m_info.m_sampling_frequency_index = 0;
    m_info.m_frame_number = 0;

    m_pts_prev = 0;
    params.is_valid = 0;

    return UMC_OK;
  }

 static AudioStreamType ObjectIDType(	unsigned long idType)
 {
    switch(idType)
    {
        case 0xe0:
            return UMC::PCM_AUDIO;
        case 0xe2:
            return UMC::AC3_AUDIO;
        case 0x05:
        case 0x07:
            return UMC::TWINVQ_AUDIO;
        case 0x6b:
            return UMC::MPEG1_AUDIO;
        case 0x69:
            return UMC::MPEG2_AUDIO;
        case 0xe1:
            return UMC::VORBIS_AUDIO;
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x06:
            return UMC::AAC_MPEG4_STREAM;
        default:
            return UMC::UNDEF_AUDIO;
    }
 }

// sloppy implementation
static int QTESDSScanAudio
(
	unsigned char *esds, 
	long count, 
	unsigned char *codec, 
	unsigned long *audio_object_type,
	unsigned long *sampling_frequency_index, 
	unsigned long *channelConfiguration
)
{
	while (count-- && (4 != *esds))
		esds += 1;

	if (4 == *esds++) 
	{
		while (*esds++ & 0x80)
			;
		*codec = *esds++;

		esds += 12;
		if (0x05 == *esds++) 
		{
			while (*esds++ & 0x80)
				;
			*audio_object_type			= ((esds[0] & 0xF8) >> 3 );
			*sampling_frequency_index	= ((esds[0] & 0x07) << 1) | ((esds[1] & 0x80) >> 7);
			*channelConfiguration		= (esds[1] & 0x78) >> 3;
			return true;
		}
	}

	return false;
}

//***
void AACDecoderInt::SetIgnoreSBR(void)
{
	aacdecSetIgnoreFIL( state );
	//aacDecSetIgnoreFIL(&state);
}

Status AACDecoderInt::Init2( unsigned char *esds, long esdsSize )
{
 	unsigned char codec;
	unsigned long audio_object_type;
	unsigned long sampling_frequency_index;
	unsigned long channelConfiguration;

	int succeed = QTESDSScanAudio
	(
		esds, 
		esdsSize, 
		&codec, 
		&audio_object_type,
		&sampling_frequency_index, 
		&channelConfiguration
	);
	
	if( !succeed )
      return UMC_NULL_PTR;//***

    aacdecInit(&state);

    initSubtype							= 0;
    m_info.m_init_stream_type			= ObjectIDType( audio_object_type );
    m_info.m_init_config_data_size		= 0;
    m_info.m_frame_number				= 0;
    m_pts_prev							= 0;
    params.is_valid						= 0;
 
    m_info.m_audio_object_type			= (AudioObjectType)audio_object_type;
    m_info.m_sampling_frequency_index	= sampling_frequency_index;
    m_info.m_decode_mode				= DM_RAW_STREAM;
  //***  m_info.m_audio_object_type			= audio_object_type;

	aacdecSetAudioObjectType(m_info.m_audio_object_type, state);
    aacdecSetSamplingFrequency(m_info.m_sampling_frequency_index, state);

    return UMC_OK;
  }

/********************************************************************/

  Status  AACDecoderInt::SetParams(BaseCodecParams_V51 * params) {
    AACDecoderParams *info = DynamicCast < AACDecoderParams > (params);

    if (info) {
      if ((info->ModeDecodeHEAACprofile == HEAAC_HQ_MODE ||
           info->ModeDecodeHEAACprofile == HEAAC_LP_MODE) &&
          (info->ModeDwnmxHEAACprofile == HEAAC_DWNSMPL_ON ||
           info->ModeDwnmxHEAACprofile == HEAAC_DWNSMPL_OFF)) {

        aacdecSetSBRModeDecode(info->ModeDecodeHEAACprofile, state);
        aacdecSetSBRModeDwnmx(info->ModeDwnmxHEAACprofile, state);

      } else
        return UMC_UNSUPPORTED;

    } else {
      return UMC_NOT_INITIALIZED;
    }

    return UMC_OK;
  }

/********************************************************************/

  Status  AACDecoderInt::GetFrame(MediaData_V51 * in,
                                  MediaData_V51 * out)
  {
    AACStatus result;
    sBitsreamBuffer BS;
    int     res;
    int     nDecodedBytes, tmp_decodedBytes;
    int     firstTime;
    sAudio_specific_config audio_config_data;
    unsigned char *inPointer;
    int inDataSize;
    double  pts_start;
    double  pts_end;
    int SbrFlagPresent, frameSize, freq, ch, decodedBytes;

    if (!in || !out)
      return UMC_NULL_PTR;

	pts_start = in->GetTime();
    if (AAC_MPEG4_STREAM != m_info.m_init_stream_type) {
      inPointer = (unsigned char *)in->GetDataPointer();
      inDataSize = in->GetDataSize();

      if (inDataSize == 0)
        return UMC_NOT_ENOUGH_DATA;

      result = aacdec_GetID3Len(inPointer, inDataSize, state);

      if (result != AAC_OK)
        return StatusAAC_2_UMC(result);

      result = aacdec_SkipID3(inDataSize, &decodedBytes, state);
      in->MoveDataPointer(decodedBytes);

      if (result != AAC_OK) {
        return StatusAAC_2_UMC(result);
      }
    }

    inPointer = (unsigned char *)in->GetDataPointer();
    GET_INIT_BITSTREAM(&BS, inPointer)
    firstTime = 0;

    while ((DM_UNDEF_STREAM == m_info.m_decode_mode) && (0 == firstTime)) {
      firstTime = 1;
      m_info.m_audio_object_type = AOT_UNDEF;
      if (AAC_MPEG4_STREAM == m_info.m_init_stream_type) {
        dec_audio_specific_config(&audio_config_data, &BS);
        /* **************************************************** *
         * if MP4 contains explicit HEAAC signalization
         * then usrParam <ModeDwnmxHEAACprofile> is ignored
         * **************************************************** */
        if( 5 == audio_config_data.extensionAudioObjectType ){ //AOT_SBR = 5
          if( audio_config_data.extensionSamplingFrequencyIndex == audio_config_data.samplingFrequencyIndex ){
            aacdecSetSBRModeDwnmx(HEAAC_DWNSMPL_ON, state);
          } else {
            aacdecSetSBRModeDwnmx(HEAAC_DWNSMPL_OFF, state);
          }
        }
        /* END SET_PARAM OF HEAAC FROM MP4 HEADER */

        m_info.m_sampling_frequency_index =
          get_sampling_frequency_index(&audio_config_data);

        m_info.m_decode_mode = DM_RAW_STREAM;

        m_info.m_audio_object_type =
          (AudioObjectType) audio_config_data.audioObjectType;

        aacdecSetAudioObjectType(m_info.m_audio_object_type, state);

        /* Init tables */
        aacdecSetSamplingFrequency(m_info.m_sampling_frequency_index, state);
        in->MoveDataPointer(m_info.m_init_config_data_size);
        out->SetDataSize(0);

        /* IMPORTANT!!! */
        return UMC_NOT_FIND_SYNCWORD;
      }

      bs_save(&BS);
      res = dec_adif_header(&m_adif_header, &BS);
      if (res == 0) {
        sProgram_config_element *m_p_pce = &m_adif_header.pce[0];

        aacdecSetPCE(m_p_pce, state);
        m_info.m_sampling_frequency_index = m_p_pce->sampling_frequency_index;
        m_info.m_decode_mode = DM_RAW_STREAM;

        /* Init tables */
        aacdecSetSamplingFrequency(m_info.m_sampling_frequency_index, state);

        switch (m_p_pce->object_type) {
        case 0:
          m_info.m_audio_object_type = AOT_AAC_MAIN;
          break;
        case 1:
          m_info.m_audio_object_type = AOT_AAC_LC;
          break;
        case 2:
          m_info.m_audio_object_type = AOT_AAC_SSR;
          break;
        case 3:
          m_info.m_audio_object_type = AOT_AAC_LTP;
          break;
        }
        aacdecSetAudioObjectType(m_info.m_audio_object_type, state);
        break;
      }

      bs_restore(&BS);

      if ((0 == dec_adts_fixed_header(&m_adts_fixed_header, &BS)) &&
          (0 == dec_adts_variable_header(&m_adts_variable_header, &BS))) {
        m_info.m_sampling_frequency_index =
          m_adts_fixed_header.sampling_frequency_index;

        m_info.m_audio_object_type = (AudioObjectType)
          get_audio_object_type_by_adts_header(&m_adts_fixed_header);

        m_info.m_decode_mode = DM_ADTS_STREAM;

        aacdecSetAudioObjectType(m_info.m_audio_object_type, state);

        /* Init tables */
        aacdecSetSamplingFrequency(m_info.m_sampling_frequency_index, state);

        adts_sw = (0xFFF << 12) + ((m_adts_fixed_header.ID & 1) << 11) +
                                  ((m_adts_fixed_header.Layer & 3) << 8) +
                                  ((m_adts_fixed_header.Profile & 3) << 6) +
                                  ((m_adts_fixed_header.sampling_frequency_index & 15) << 2);

        bs_restore(&BS);
        break;
      }
      bs_restore(&BS);

      m_info.m_stream_subtype = UNDEF_AUDIO_SUBTYPE;

      if (m_info.m_audio_object_type == AOT_AAC_MAIN) {
        m_info.m_stream_subtype = AAC_MAIN_PROFILE;
      } else if (m_info.m_audio_object_type == AOT_AAC_LC) {
        m_info.m_stream_subtype = AAC_LC_PROFILE;
      } else if (m_info.m_audio_object_type == AOT_AAC_SSR) {
        m_info.m_stream_subtype = AAC_SSR_PROFILE;
      } else if (m_info.m_audio_object_type == AOT_AAC_LTP) {
        m_info.m_stream_subtype = AAC_LTP_PROFILE;
      }
    }

    if (AOT_UNDEF == m_info.m_audio_object_type)
      return UMC_UNSUPPORTED;

    if (DM_ADTS_STREAM == m_info.m_decode_mode) {
      int DataSize = in->GetDataSize();
      unsigned char *tmp_ptr = (unsigned char *)in->GetDataPointer();
      unsigned int val;

      if (DataSize < 9)
        return UMC_NOT_ENOUGH_DATA;

      val = (tmp_ptr[0] << 16) + (tmp_ptr[1] << 8) + tmp_ptr[2];
      DataSize -= 3;
      tmp_ptr += 3;

      while (((val & 0xFFFEFC) != adts_sw) && (DataSize > 0)) {
        val = (val << 16) + tmp_ptr[0];
        DataSize--;
        tmp_ptr++;
      }

      if ((val & 0xFFFEFC) != adts_sw) {
        in->MoveDataPointer(in->GetDataSize()-2);
        return UMC_NOT_ENOUGH_DATA;
      }

      DataSize += 3;
      in->MoveDataPointer(in->GetDataSize()-DataSize);

      if (DataSize < 9) {
        return UMC_NOT_ENOUGH_DATA;
      }

      inPointer = (unsigned char *)in->GetDataPointer();
      GET_INIT_BITSTREAM(&BS, inPointer)

      dec_adts_fixed_header(&m_adts_fixed_header, &BS);
      dec_adts_variable_header(&m_adts_variable_header, &BS);

      if (m_adts_fixed_header.protection_absent == 0) {
        GET_BITS(&BS, val, 16)
      }

      if (m_adts_variable_header.aac_frame_length > DataSize) {
        return UMC_NOT_ENOUGH_DATA;
      }

      aacdecSetAdtsChannelConfiguration(m_adts_fixed_header.channel_configuration,
                                        state);
    }

    Byte_alignment(&BS);
    GET_BITS_COUNT(&BS, tmp_decodedBytes)
    tmp_decodedBytes >>= 3;
    nDecodedBytes = 0;

    result = aacdecGetFrame(inPointer + tmp_decodedBytes, &nDecodedBytes,
                            (Ipp16s *)out->GetDataPointer(), state);

    nDecodedBytes += tmp_decodedBytes;

    aacdecGetSbrFlagPresent(&SbrFlagPresent, state);
    aacdecGetFrameSize(&frameSize, state);
    aacdecGetSampleFrequency(&freq, state);
    aacdecGetChannels(&ch, state);

    if ((0 == initSubtype) && (1 == SbrFlagPresent)) {
      initSubtype = 1;
      if (m_info.m_stream_subtype == AAC_MAIN_PROFILE) {
        m_info.m_stream_subtype =
          (UMC::AudioStreamSubType) (AAC_MAIN_PROFILE | AAC_HE_PROFILE);
      } else if (m_info.m_stream_subtype == AAC_LC_PROFILE) {
        m_info.m_stream_subtype =
          (UMC::AudioStreamSubType) (AAC_LC_PROFILE | AAC_HE_PROFILE);
      } else if (m_info.m_stream_subtype == AAC_SSR_PROFILE) {
        m_info.m_stream_subtype =
          (UMC::AudioStreamSubType) (AAC_SSR_PROFILE | AAC_HE_PROFILE);
      } else if (m_info.m_stream_subtype == AAC_LTP_PROFILE) {
        m_info.m_stream_subtype =
          (UMC::AudioStreamSubType) (AAC_LTP_PROFILE | AAC_HE_PROFILE);
      }
    }

    if (AAC_MPEG4_STREAM == m_info.m_init_stream_type) {
      in->MoveDataPointer(in->GetDataSize());
    } else {
      in->MoveDataPointer(nDecodedBytes);
    }

    if (AAC_BAD_STREAM == result) {
      int size = frameSize * sizeof(short) * ch;
      //int size = max(frameSize * sizeof(short) * ch, (int)out->GetDataSize());

      // prevent from crash
      //size = min(size, (int)out->GetBufferSize());

      memset(out->GetDataPointer(),0,size);

      // fill with silent data
      out->SetDataSize(size);

      if (pts_start < 0)
        pts_start = m_pts_prev;

      m_pts_prev = pts_end = pts_start +
        ((double)size / (double)freq);

      in->SetTime(pts_end);
      out->SetTime(pts_start, pts_end);

      m_info.m_frame_number++;

      result = AAC_OK;
    } else if (AAC_OK == result) {
      AudioData_V51* pAudio = DynamicCast<AudioData_V51,MediaData_V51>(out);

      out->SetDataSize(frameSize * sizeof(short) * ch);

      if (pts_start < 0)
        pts_start = m_pts_prev;

      m_pts_prev = pts_end = pts_start +
        ((double)frameSize / (double)freq);

      in->SetTime(pts_end);
      out->SetTime(pts_start, pts_end);

      m_info.m_frame_number++;

      if (pAudio) {
        pAudio->m_info.bitPerSample = 16;
        pAudio->m_info.bitrate = 0;

        pAudio->m_info.channels = ch;
        pAudio->m_info.sample_frequency = freq;
        pAudio->m_info.stream_type = PCM_AUDIO;

        switch (pAudio->m_info.channels) {
        case 1:
          pAudio->m_info.channel_mask = CHANNEL_FRONT_CENTER;
          break;
        case 2:
          pAudio->m_info.channel_mask  = CHANNEL_FRONT_LEFT;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_RIGHT;
          break;
        case 3:
          pAudio->m_info.channel_mask  = CHANNEL_FRONT_CENTER;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_LEFT;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_RIGHT;
          break;
        case 4:
          pAudio->m_info.channel_mask  = CHANNEL_FRONT_CENTER;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_LEFT;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_RIGHT;
          pAudio->m_info.channel_mask |= CHANNEL_LOW_FREQUENCY;
          break;
        case 5:
          pAudio->m_info.channel_mask  = CHANNEL_FRONT_CENTER;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_LEFT;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_RIGHT;
          pAudio->m_info.channel_mask |= CHANNEL_BACK_LEFT;
          pAudio->m_info.channel_mask |= CHANNEL_BACK_RIGHT;
          break;
        case 6:
          pAudio->m_info.channel_mask  = CHANNEL_FRONT_CENTER;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_LEFT;
          pAudio->m_info.channel_mask |= CHANNEL_FRONT_RIGHT;
          pAudio->m_info.channel_mask |= CHANNEL_BACK_LEFT;
          pAudio->m_info.channel_mask |= CHANNEL_BACK_RIGHT;
          pAudio->m_info.channel_mask |= CHANNEL_LOW_FREQUENCY;
          break;
        default:
          break;
        }
      }
    }

    return StatusAAC_2_UMC(result);
  }

/********************************************************************/

  AACDecoderInt::~AACDecoderInt()
  {
    Close();
  }

/********************************************************************/

  Status AACDecoderInt::Close()
  {
    aacdecClose(state);
    state = NULL;
    return UMC_OK;
  }

/********************************************************************/

  Status  AACDecoderInt::Reset()
  {
    aacdecReset(state);
    return UMC_OK;
  }

/********************************************************************/

  Status  AACDecoderInt::GetInfo(BaseCodecParams_V51 * info)
  {
    if (!info)
      return UMC_NULL_PTR;

    AudioCodecParams_V51 *p_info =
      DynamicCast < AudioCodecParams_V51, BaseCodecParams_V51 > (info);

    aacdecGetInfo(&params, state);

    info->m_SuggestedInputSize = params.m_SuggestedInputSize;

    if (params.is_valid) {
      p_info->m_info_in.bitPerSample = params.m_info_in.bitPerSample;
      p_info->m_info_out.bitPerSample = params.m_info_out.bitPerSample;

      p_info->m_info_in.channels = params.m_info_in.channels;
      p_info->m_info_out.channels = params.m_info_out.channels;

      p_info->m_info_in.stream_type = AAC_AUDIO;
      p_info->m_info_out.stream_type = PCM_AUDIO;

      p_info->m_info_in.sample_frequency = params.m_info_in.sample_frequency;
      p_info->m_info_out.sample_frequency = params.m_info_out.sample_frequency;

      p_info->m_frame_num = m_info.m_frame_number;

      p_info->m_info_in.bitrate = params.m_info_in.bitrate;
      p_info->m_info_out.bitrate = params.m_info_out.bitrate;

      p_info->m_info_in.stream_subtype = m_info.m_stream_subtype;

      switch (p_info->m_info_in.channels) {
      case 1:
        p_info->m_info_in.channel_mask  = CHANNEL_FRONT_CENTER;
        break;
      case 2:
        p_info->m_info_in.channel_mask  = CHANNEL_FRONT_LEFT;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_RIGHT;
        break;
      case 3:
        p_info->m_info_in.channel_mask  = CHANNEL_FRONT_CENTER;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_LEFT;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_RIGHT;
        break;
      case 4:
        p_info->m_info_in.channel_mask  = CHANNEL_FRONT_CENTER;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_LEFT;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_RIGHT;
        p_info->m_info_in.channel_mask |= CHANNEL_LOW_FREQUENCY;
        break;
      case 5:
        p_info->m_info_in.channel_mask  = CHANNEL_FRONT_CENTER;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_LEFT;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_RIGHT;
        p_info->m_info_in.channel_mask |= CHANNEL_BACK_LEFT;
        p_info->m_info_in.channel_mask |= CHANNEL_BACK_RIGHT;
        break;
      case 6:
        p_info->m_info_in.channel_mask  = CHANNEL_FRONT_CENTER;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_LEFT;
        p_info->m_info_in.channel_mask |= CHANNEL_FRONT_RIGHT;
        p_info->m_info_in.channel_mask |= CHANNEL_BACK_LEFT;
        p_info->m_info_in.channel_mask |= CHANNEL_BACK_RIGHT;
        p_info->m_info_in.channel_mask |= CHANNEL_LOW_FREQUENCY;
        break;
      default:
        break;
      }

      p_info->m_info_out.channel_mask = p_info->m_info_in.channel_mask;
    }

    return UMC_OK;
  }

/********************************************************************/

  Status AACDecoderInt::GetDuration(float *p_duration)
  {
    aacdecGetDuration(p_duration, state);
    return UMC_OK;
  }

/********************************************************************/

  Status AACDecoderInt::StatusAAC_2_UMC(AACStatus st)
  {
    Status res;
    if (st == AAC_OK)
      res = UMC_OK;
    else if (st == AAC_NOT_ENOUGH_DATA)
      res = UMC_NOT_ENOUGH_DATA;
    else if (st == AAC_BAD_FORMAT)
      res = UMC_BAD_FORMAT;
    else if (st == AAC_ALLOC)
      res = UMC_ALLOC;
    else if (st == AAC_BAD_STREAM)
      res = UMC_BAD_STREAM;
    else if (st == AAC_NULL_PTR)
      res = UMC_NULL_PTR;
    else if (st == AAC_NOT_FIND_SYNCWORD)
      res = UMC_NOT_FIND_SYNCWORD;
    else if (st == AAC_UNSUPPORTED)
      res = UMC_UNSUPPORTED;
    else if (st == AAC_BAD_PARAMETER)
      res = UMC_UNSUPPORTED;
    else
      res = UMC_UNSUPPORTED;

    return res;
  }
};      // namespace UMC

