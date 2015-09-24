/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "mp3dec_own.h"

int mp3dec_audio_data_LayerII(MP3Dec_com *state)
{
    int gr, ch, sb;
    short *degroup_table;
    short value;
    int jsbound = state->jsbound;
    int stereo = state->stereo;
    sBitsreamBuffer *m_StreamData = &(state->m_StreamData);
    short (*allocation)[32] = state->allocation;
    int           (*sample)[32][36] = state->sample;
    int           *nbal_alloc_table = state->nbal_alloc_table;
    int           sblimit = state->sblimit;
    short         (*scfsi)[32] = state->scfsi;
    short         (*scalefactor)[3][32] = state->scalefactor;
    unsigned char *alloc_table = state->alloc_table;

    for (sb = 0; sb < jsbound; sb++) {
        for (ch = 0; ch < stereo; ch++) {
            GET_BITS(m_StreamData, allocation[ch][sb], nbal_alloc_table[sb]);
        }
    }

    for (sb = jsbound; sb < sblimit; sb++) {
        GET_BITS(m_StreamData, allocation[0][sb], nbal_alloc_table[sb]);
        allocation[1][sb] = allocation[0][sb];
    }

    for (sb = 0; sb < sblimit; sb++) {
        for (ch = 0; ch < stereo; ch++) {
            if (allocation[ch][sb]) {
                GET_BITS(m_StreamData, scfsi[ch][sb], 2);
            }
        }
    }

    for (sb = 0; sb < sblimit; sb++) {
        for (ch = 0; ch < stereo; ch++) {
            if (allocation[ch][sb] != 0) {
                if (scfsi[ch][sb] == 0) {
                    GET_BITS(m_StreamData, scalefactor[ch][0][sb], 6);
                    GET_BITS(m_StreamData, scalefactor[ch][1][sb], 6);
                    GET_BITS(m_StreamData, scalefactor[ch][2][sb], 6);
                } else if (scfsi[ch][sb] == 1) {
                    GET_BITS(m_StreamData, scalefactor[ch][0][sb], 6);
                    scalefactor[ch][1][sb] = scalefactor[ch][0][sb];
                    GET_BITS(m_StreamData, scalefactor[ch][2][sb], 6);
                } else if (scfsi[ch][sb] == 2) {
                    GET_BITS(m_StreamData, scalefactor[ch][2][sb], 6);
                    scalefactor[ch][0][sb] =
                        scalefactor[ch][1][sb] =
                        scalefactor[ch][2][sb];
                } else if (scfsi[ch][sb] == 3) {
                    GET_BITS(m_StreamData, scalefactor[ch][0][sb], 6);
                    GET_BITS(m_StreamData, scalefactor[ch][2][sb], 6);
                    scalefactor[ch][1][sb] =
                        scalefactor[ch][2][sb];
                }
            } else {
                scalefactor[ch][0][sb] =
                    scalefactor[ch][1][sb] =
                    scalefactor[ch][2][sb] = 63;
            }
        }
    }

    for (gr = 0; gr < 12; gr++) {
        for (sb = 0; sb < sblimit; sb++) {
            for (ch = 0; ch < ((sb < jsbound) ? stereo : 1); ch++) {
                if (allocation[ch][sb] != 0) {
                    int idx = alloc_table[sb * 16 + allocation[ch][sb]];
                    int c;

                    idx = mp3dec_cls_quant[idx];
                    if (idx < 0) {
                        idx = - idx;
                        degroup_table = (short *)mp3dec_degroup[idx];
                        GET_BITS(m_StreamData, c, idx);
                        value = degroup_table[c];
                        sample[ch][sb][3 * gr + 0] = value >> 8;
                        sample[ch][sb][3 * gr + 1] = (value >> 4) & 0xF;
                        sample[ch][sb][3 * gr + 2] = value & 0xF;
                    } else {
                        GET_BITS(m_StreamData, sample[ch][sb][3 * gr + 0], idx);
                        GET_BITS(m_StreamData, sample[ch][sb][3 * gr + 1], idx);
                        GET_BITS(m_StreamData, sample[ch][sb][3 * gr + 2], idx);
                    }

                    // joint stereo : copy L to R
                    if (stereo == 2 && sb >= jsbound) {
                        sample[1][sb][3 * gr + 0] = sample[0][sb][3 * gr + 0];
                        sample[1][sb][3 * gr + 1] = sample[0][sb][3 * gr + 1];
                        sample[1][sb][3 * gr + 2] = sample[0][sb][3 * gr + 2];
                    }
                } else {
                    sample[ch][sb][3 * gr + 0] = 0;
                    sample[ch][sb][3 * gr + 1] = 0;
                    sample[ch][sb][3 * gr + 2] = 0;
                }
            }
        }
    }
    return 1;
}
