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

int mp3dec_audio_data_LayerI(MP3Dec_com *state)
{
    int gr, ch, sb;
    int jsbound = state->jsbound;
    int stereo = state->stereo;
    sBitsreamBuffer *m_StreamData = &(state->m_StreamData);
    short (*allocation)[32] = state->allocation;
    int           (*sample)[32][36] = state->sample;
    short         (*scalefactor)[32] = state->scalefactor_l1;

    for (sb = 0; sb < jsbound; sb++) {
        for (ch = 0; ch < stereo; ch++) {
            GET_BITS(m_StreamData, allocation[ch][sb], 4);
        }
    }

    for (sb = jsbound; sb < 32; sb++) {
        GET_BITS(m_StreamData, allocation[0][sb], 4);
        allocation[1][sb] = allocation[0][sb];
    }

    for (sb = 0; sb < 32; sb++) {
        for (ch = 0; ch < stereo; ch++) {
            if (allocation[ch][sb]!=0) {
                GET_BITS(m_StreamData, scalefactor[ch][sb], 6);
            }
        }
    }

    for (gr = 0; gr < 12; gr++) {
        for (sb = 0; sb < 32; sb++) {
            for (ch = 0; ch < ((sb < jsbound) ? stereo : 1); ch++) {
                if (allocation[ch][sb] != 0) {
                    int idx = allocation[ch][sb] + 1;
                    int c;

                    GET_BITS(m_StreamData, c, idx);

                    sample[ch][sb][gr] = c;

                    // joint stereo : copy L to R
                    if (stereo == 2 && sb >= jsbound) {
                        sample[1][sb][gr] = sample[0][sb][gr];
                    }
                }
            }
        }
    }
    return 1;
}
