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

#include "mp3dec_own_int.h"

MP3Status mp3decInit(MP3Dec **state_ptr)
{
    MP3Dec *state;
//    int size, size_short, size_dct;

    state = (MP3Dec *)ippsMalloc_8u_x(sizeof(MP3Dec));
    if (state == NULL)
        return MP3_ALLOC;

    if (mp3decInit_com(&state->com) == MP3_ALLOC) {
        ippsFree_x(state);
        return MP3_ALLOC;
    }

    ippsZero_8u_x((Ipp8u *)(state->tmp_bfr), 32 * sizeof(float));
    ippsZero_8u_x((Ipp8u *)(state->filter_bfr), 2 * 64 * 16 * sizeof(float));

    ippsZero_8u_x((Ipp8u *)(state->GlobalScaleFactor_e), 2 * 2 * sizeof(int));
    ippsZero_8u_x((Ipp8u *)(state->GlobalScaleFactor_m), 2 * 2 * sizeof(int));
/*
    state->pMDCTSpecShort = NULL;
    state->pMDCTSpecLong = NULL;
    state->pDCTSpecFB = NULL;
    state->mdct_buffer = NULL;

    if (ippsMDCTInvInitAlloc_32f(&(state->pMDCTSpecLong), 36) == ippStsMemAllocErr) {
        ippsFree_x(state->com.m_MainData.pBuffer);
        ippsFree_x(state);
        return MP3_ALLOC;
    }
    if (ippsMDCTInvInitAlloc_32f(&(state->pMDCTSpecShort), 12) == ippStsMemAllocErr) {
        ippsMDCTInvFree_32f(state->pMDCTSpecLong);
        ippsFree_x(state->com.m_MainData.pBuffer);
        ippsFree_x(state);
        return MP3_ALLOC;
    }
    if (ippsDCTFwdInitAlloc_32f(&(state->pDCTSpecFB), 32, ippAlgHintNone)== ippStsMemAllocErr) {
        ippsMDCTInvFree_32f(state->pMDCTSpecShort);
        ippsMDCTInvFree_32f(state->pMDCTSpecLong);
        ippsFree_x(state->com.m_MainData.pBuffer);
        ippsFree_x(state);
        return MP3_ALLOC;
    }

    ippsMDCTInvGetBufSize_32f(state->pMDCTSpecLong, &size);
    ippsMDCTInvGetBufSize_32f(state->pMDCTSpecShort, &size_short);
    ippsDCTFwdGetBufSize_32f(state->pDCTSpecFB, &size_dct);

    if (size < size_short) size = size_short;
    if (size < size_dct) size = size_dct;
    state->mdct_buffer = ippsMalloc_8u_x(size);

    if (state->mdct_buffer == NULL) {
        ippsMDCTInvFree_32f(state->pMDCTSpecShort);
        ippsMDCTInvFree_32f(state->pMDCTSpecLong);
        ippsDCTFwdFree_32f(state->pDCTSpecFB);
        ippsFree_x(state->com.m_MainData.pBuffer);
        ippsFree_x(state);
        return MP3_ALLOC;
    }
*/
    ippsZero_8u_x((Ipp8u *)(state->global), 5760 * sizeof(unsigned int));
    ippsZero_8u_x((Ipp8u *)(state->prevblk), 2 * 576 * sizeof(float));

    /* out of subband synth */
    state->smpl_sb = (sampleshort *) state->global;
    /* out of dequantizer */
    state->smpl_xr =
    /* out of reordering */
    state->smpl_ro =
    /* out of antialiasing */
    state->smpl_re = (sampleint *) state->smpl_sb;
    /* out of imdct */
    state->smpl_rw = (sampleintrw *) ((char *)(state->global) + sizeof(sampleint));
    /* out of huffman */
    state->com.smpl_xs = (sampleshort *) ((char *)(state->global) +
        sizeof(sampleint) * 2);

    ippsZero_8u_x((Ipp8u *)(state->m_ptr), 2 * 2 * sizeof(short));
    state->m_even[0] = state->m_even[1] = 0;

    state->synth_ind[0] = state->synth_ind[1] = 0;
    ippsZero_8u_x((Ipp8u *)(state->synth_buf), IPP_MP3_V_BUF_LEN * 2 * sizeof(Ipp32s));
    state->dctnum_prev[0] = state->dctnum_prev[1] = 0;

    *state_ptr = state;

    return MP3_OK;
}

MP3Status mp3decReset(MP3Dec *state)
{
    mp3decReset_com(&state->com);

    ippsZero_8u_x((Ipp8u *)(state->tmp_bfr), 64 * sizeof(float));
    ippsZero_8u_x((Ipp8u *)(state->filter_bfr), 2 * 64 * 16 * sizeof(float));

    ippsZero_8u_x((Ipp8u *)(state->GlobalScaleFactor_e), 2 * 2 * sizeof(int));
    ippsZero_8u_x((Ipp8u *)(state->GlobalScaleFactor_m), 2 * 2 * sizeof(int));

    ippsZero_8u_x((Ipp8u *)(state->global), 5760 * sizeof(unsigned int));
    ippsZero_8u_x((Ipp8u *)(state->prevblk), 2 * 576 * sizeof(float));

    ippsZero_8u_x((Ipp8u *)(state->m_ptr), 2 * 2 * sizeof(short));
    state->m_even[0] = state->m_even[1] = 0;

    return MP3_OK;
}

MP3Status mp3decClose(MP3Dec *state)
{
    if (state == NULL)
        return MP3_OK;

/*    if (state->pMDCTSpecShort) {
        ippsMDCTInvFree_32f(state->pMDCTSpecShort);
        state->pMDCTSpecShort = NULL;
    }
    if (state->pMDCTSpecLong) {
        ippsMDCTInvFree_32f(state->pMDCTSpecLong);
        state->pMDCTSpecLong = NULL;
    }
    if (state->pDCTSpecFB) {
        ippsDCTFwdFree_32f(state->pDCTSpecFB);
        state->pDCTSpecFB = NULL;
    }
    if (state->mdct_buffer) {
        ippsFree_x(state->mdct_buffer);
        state->mdct_buffer = NULL;
    }
*/
    mp3decClose_com(&state->com);
    ippsFree_x(state);

    return MP3_OK;
}


MP3Status mp3decGetFrame(Ipp8u *inPointer,
                         int inDataSize,
                         int *decodedBytes,
                         Ipp16s *outPointer,
                         int outBufferSize,
                         MP3Dec *state)
{
    MP3Status  res;
    int     prev_decodedBytes = state->com.decodedBytes;
    int     frameSize;

    IppMP3FrameHeader *header = &(state->com.header);

    if (!inPointer || !outPointer)
        return MP3_NULL_PTR;
    res = mp3dec_GetID3Len(inPointer, inDataSize, &(state->com));

    if (res != MP3_OK)
      return res;

    res = mp3dec_SkipID3(inDataSize, decodedBytes, &(state->com));

    if (res != MP3_OK) {
      return res;
    } else {
      inDataSize -= *decodedBytes;
      inPointer += *decodedBytes;
    }

    if (inDataSize == 0 && (state->com.m_StreamData.nDataLen <= state->com.decodedBytes + 32))
        return MP3_NOT_ENOUGH_DATA;
    state->com.m_pOutSamples = (Ipp16s *)outPointer;

    res = mp3dec_ReceiveBuffer(&(state->com.m_StreamData), inPointer, inDataSize);

    if (res != MP3_OK)
        return res;

    do {
        res = mp3dec_GetSynch(&state->com);
        if (res == MP3_NOT_FIND_SYNCWORD || res == MP3_BAD_STREAM || res == MP3_UNSUPPORTED) {
            *decodedBytes += state->com.decodedBytes - prev_decodedBytes;
            return res;
        } else if (res == MP3_NOT_ENOUGH_DATA) {
            return res;
        }

        mp3decGetFrameSize(&frameSize, state);

        if (outBufferSize < frameSize << (state->com.stereo - 1)) {
            return MP3_NOT_ENOUGH_BUFFER;
        }

        //CMC END
        state->com.m_frame_num++;

        res = MP3_OK;

        switch (header->layer) {
        case 3:
            if (header->id)
                mp3dec_audio_data_LayerIII(&state->com);
            else
                mp3dec_audio_data_LSF_LayerIII(&state->com);
            res = mp3dec_decode_data_LayerIII(state);
            break;
        case 2:
            if (state->com.nbal_alloc_table) {
                mp3dec_audio_data_LayerII(&state->com);
                mp3dec_decode_data_LayerII(state);
            }
            break;
        case 1:
            mp3dec_audio_data_LayerI(&state->com);
            mp3dec_decode_data_LayerI(state);
            break;

        default:
            res = MP3_UNSUPPORTED;  // unsupported layer
        }
    } while (0/*res == MP3_NOT_FIND_SYNCWORD*/);

    *decodedBytes += (state->com.decodedBytes - prev_decodedBytes);

    ippsCopy_8u_x((Ipp8u *)&state->com.header, (Ipp8u *)&state->com.header_good,
        sizeof(IppMP3FrameHeader));
    state->com.mpg25_good = state->com.mpg25;
    state->com.m_bInit = 1;

    return res;
}

MP3Status mp3decGetInfo(cAudioCodecParams *a_info, MP3Dec *state)
{
    IppMP3FrameHeader *header;
    int     ch[] = { 2, 2, 2, 1 };
    cAudioStreamType mpeg_type[] = { MPEG2_AUD, MPEG1_AUD };
    cAudioStreamType layer_type[] =
    { MPEG_AUD_LAYER1, MPEG_AUD_LAYER2, MPEG_AUD_LAYER3 };

    if (!a_info)
        return MP3_NULL_PTR;

    header = &(state->com.header_good);

    a_info->m_SuggestedInputSize = 2048;

    if (state->com.m_bInit) {
        a_info->m_info_in.bitPerSample = 0;
        a_info->m_info_out.bitPerSample = 16;

        a_info->m_info_in.bitrate =
            mp3dec_bitrate[header->id][3 - header->layer][header->bitRate] * 1000;
        a_info->m_info_out.bitrate = 0;

        a_info->m_info_in.channels = ch[header->mode];
        a_info->m_info_out.channels = ch[header->mode];

        a_info->m_info_in.sample_frequency =
            mp3dec_frequency[header->id + state->com.mpg25_good][header->samplingFreq];
        a_info->m_info_out.sample_frequency =
            mp3dec_frequency[header->id + state->com.mpg25_good][header->samplingFreq];

        a_info->m_info_in.stream_type = (cAudioStreamType)
            (mpeg_type[header->id] | layer_type[header->layer - 1]);
        a_info->m_info_out.stream_type = PCM_AUD;

        a_info->m_frame_num = state->com.m_frame_num;

        a_info->is_valid = 1;

        return MP3_OK;
    }

    a_info->is_valid = 0;

    return MP3_OK;
}

MP3Status mp3decGetDuration(float *p_duration, MP3Dec *state)
{
    float   duration;
    int frameSize;
    IppMP3FrameHeader *header;

    if (!state)
        return MP3_NULL_PTR;

    header = &(state->com.header_good);

    mp3decGetFrameSize(&frameSize, state);
    duration = (float)state->com.m_frame_num * frameSize;

    duration /= (float)mp3dec_frequency[header->id + state->com.mpg25_good][header->samplingFreq];

    *p_duration = duration;

    return MP3_OK;
}

MP3Status mp3decGetChannels(int *ch, MP3Dec *state)
{
    IppMP3FrameHeader *header;
    int     ch_table[] = { 2, 2, 2, 1 };

    if (!state)
        return MP3_NULL_PTR;

    header = &(state->com.header_good);

    *ch = ch_table[header->mode];

    return MP3_OK;
}

MP3Status mp3decGetFrameSize(int *frameSize, MP3Dec *state)
{
    IppMP3FrameHeader *header;
    int     fs[2][4] = {
        { 0, 384, 1152,  576 },
        { 0, 384, 1152, 1152 }
    };

    if (!state)
        return MP3_NULL_PTR;

    header = &(state->com.header_good);

    *frameSize = fs[header->id][header->layer];

    return MP3_OK;
}

MP3Status mp3decGetSampleFrequency(int *freq, MP3Dec *state)
{
    IppMP3FrameHeader *header;

    if (!state)
        return MP3_NULL_PTR;

    header = &(state->com.header_good);

    *freq = mp3dec_frequency[header->id + state->com.mpg25_good][header->samplingFreq];

    return MP3_OK;
}
