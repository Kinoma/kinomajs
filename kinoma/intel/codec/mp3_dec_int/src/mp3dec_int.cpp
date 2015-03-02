/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

//***
#include "kinoma_ipp_lib.h"
#include "ipps.h"

#include "umc_mp3_dec.h"

namespace UMC {

    MP3DecoderInt::MP3DecoderInt()
    {
        state = NULL;
    }

    MP3DecoderInt::~MP3DecoderInt()
    {
        Close();
    }

    Status MP3DecoderInt::Init(BaseCodecParams_V51 * init)
    {
        mp3decInit(&state);

        params.is_valid = 0;
        m_frame_num = 0;
        m_pts_prev = 0;

        return UMC_OK;
    }

    Status MP3DecoderInt::GetFrame(MediaData_V51* in, MediaData_V51* out)
    {
        MP3Status resMP3;
        int     shift = 0;
        int     frameSize, freq, ch;
        Ipp8u *in_ptr;
        Ipp16s *out_ptr;
        int     in_size, out_size;

        if (!in || !in->GetDataPointer() || !out || !out->GetDataPointer())
            return UMC_NULL_PTR;

        in_ptr = (Ipp8u *)in->GetDataPointer();
        in_size = in->GetDataSize();
        out_size = out->GetBufferSize() / sizeof(Ipp16s);
        out_ptr = (Ipp16s *)out->GetDataPointer();
        resMP3 = mp3decGetFrame(in_ptr, in_size, &shift, out_ptr, out_size, state);

        if (shift) {
            if ((MP3_OK == resMP3 || MP3_BAD_STREAM == resMP3)) {
                double  pts_start = in->GetTime();
                double  pts_end;

                mp3decGetFrameSize(&frameSize, state);
                mp3decGetSampleFrequency(&freq, state);
                mp3decGetChannels(&ch, state);

                if (pts_start < 0)
                    pts_start = m_pts_prev;
                m_pts_prev = pts_end =
                    pts_start +
                    (double)frameSize / (double)freq;

                in->MoveDataPointer(shift);
                in->SetTime(pts_end);

                if(MP3_BAD_STREAM == resMP3) {
                    resMP3 = MP3_OK;
                    ippsZero_16s_x(out_ptr, frameSize * ch);
                }

                out->SetDataSize(frameSize * sizeof(Ipp16s) * ch);
                out->SetTime(pts_start, pts_end);
                AudioData_V51* pAudio    = DynamicCast<AudioData_V51,MediaData_V51>(out);
                if ((MP3_OK == resMP3 && pAudio))
                {
                    pAudio->m_info.bitPerSample = 16;
                    pAudio->m_info.bitrate = 0;

                    pAudio->m_info.channels = ch;
                    pAudio->m_info.sample_frequency = freq;
                    pAudio->m_info.stream_type = PCM_AUDIO;

                }
                m_frame_num++;
            }
            else {
                in->MoveDataPointer(shift);
            }
        }

        return StatusMP3_2_UMC(resMP3);
    }

    Status MP3DecoderInt::Close()
    {
        mp3decClose(state);

        state = NULL;

        return UMC_OK;
    }

    Status MP3DecoderInt::Reset()
    {
//        params.is_valid = 0;
        m_frame_num = 0;

        mp3decReset(state);

        return UMC_OK;
    }

    Status MP3DecoderInt::GetInfo(BaseCodecParams_V51 * info)
    {
        if (!info)
            return UMC_NULL_PTR;

        AudioCodecParams_V51 *a_info = (AudioCodecParams_V51 *) info;

        mp3decGetInfo(&params, state);

        info->m_SuggestedInputSize = params.m_SuggestedInputSize;

        if (params.is_valid) {
            a_info->m_info_in.bitPerSample = params.m_info_in.bitPerSample;
            a_info->m_info_out.bitPerSample = params.m_info_out.bitPerSample;

            a_info->m_info_in.bitrate = params.m_info_in.bitrate;
            a_info->m_info_out.bitrate = params.m_info_out.bitrate;

            a_info->m_info_in.channels = params.m_info_in.channels;
            a_info->m_info_out.channels = params.m_info_out.channels;

            a_info->m_info_in.sample_frequency =
                params.m_info_in.sample_frequency;
            a_info->m_info_out.sample_frequency =
                params.m_info_out.sample_frequency;

            switch (params.m_info_in.stream_type) {
          case MP1L1_AUD:
              a_info->m_info_in.stream_type = MP1L1_AUDIO;
              break;
          case MP1L2_AUD:
              a_info->m_info_in.stream_type = MP1L2_AUDIO;
              break;
          case MP1L3_AUD:
              a_info->m_info_in.stream_type = MP1L3_AUDIO;
              break;
          case MP2L1_AUD:
              a_info->m_info_in.stream_type = MP2L1_AUDIO;
              break;
          case MP2L2_AUD:
              a_info->m_info_in.stream_type = MP2L2_AUDIO;
              break;
          case MP2L3_AUD:
              a_info->m_info_in.stream_type = MP2L3_AUDIO;
              break;
          default:
              break;
            };

            a_info->m_info_out.stream_type = PCM_AUDIO;

            a_info->m_frame_num = params.m_frame_num;

            return UMC_OK;
        }

        return UMC_OK;
    }

    Status MP3DecoderInt::GetDuration(float *p_duration)
    {
        mp3decGetDuration(p_duration, state);
        return UMC_OK;
    }

    Status MP3DecoderInt::StatusMP3_2_UMC(MP3Status st)
    {
        Status res;
        if (st == MP3_OK)
            res = UMC_OK;
        else if (st == MP3_NOT_ENOUGH_DATA)
            res = UMC_NOT_ENOUGH_DATA;
        else if (st == MP3_BAD_FORMAT)
            res = UMC_BAD_FORMAT;
        else if (st == MP3_ALLOC)
            res = UMC_ALLOC;
        else if (st == MP3_BAD_STREAM)
            res = UMC_BAD_STREAM;
        else if (st == MP3_NULL_PTR)
            res = UMC_NULL_PTR;
        else if (st == MP3_NOT_FIND_SYNCWORD)
            res = UMC_NOT_FIND_SYNCWORD;
        else if (st == MP3_NOT_ENOUGH_BUFFER)
            res = UMC_NOT_ENOUGH_BUFFER;
        else if (st == MP3_FAILED_TO_INITIALIZE)
            res = UMC_FAILED_TO_INITIALIZE;
        else if (st == MP3_UNSUPPORTED)
            res = UMC_UNSUPPORTED;
        else
            res = UMC_UNSUPPORTED;

        return res;
    }
}
