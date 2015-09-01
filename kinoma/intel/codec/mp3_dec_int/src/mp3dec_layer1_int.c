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

/* Q31 */
static const int mp3dec_l1_dequant_tabled[] = {
              0,
     1431655808,
     1227133571,
     1145324670,
     1108378624,
     1090785410,
     1082196483,
     1077952643,
     1075843072,
     1074791425,
     1074266369,
     1074004097,
     1073872895,
     1073807361,
     1073774590,
};

/* Q28 */
static const int scale_values_int[3] = {
      268435456,
      213057360,
      169103744,
};

int mp3dec_decode_data_LayerI(MP3Dec *state)
{
    int i, ch, sb;
    int           (*sample)[32][36] = state->com.sample;
    short         (*scalefactor)[32] = state->com.scalefactor_l1;
    short (*allocation)[32] = state->com.allocation;
    sampleintrw *smpl_rw = state->smpl_rw;     // out of imdct
    int stereo = state->com.stereo;
    short  *m_pOutSamples = state->com.m_pOutSamples;

    for (ch = 0; ch < stereo; ch++) {
        for (sb = 0; sb < 32; sb++) {
            if (allocation[ch][sb] != 0) {
                int idx = allocation[ch][sb] + 1;
                int xor_coef;
                int *sample_ptr = &sample[ch][sb][0];
                int scale_fact, scale_mul, scale_shift, scale_mod;

                xor_coef = (1 << (idx - 1));
                scale_fact = scalefactor[ch][sb];
                scale_shift = scale_fact / 3;
                scale_mod = scale_fact - scale_shift * 3;
                scale_mul = MUL32_MP3_32S(scale_values_int[scale_mod],
                    mp3dec_l1_dequant_tabled[allocation[ch][sb]]);

                for (i = 0; i < 12; i++) {
                    int smpl = ((sample_ptr[i] ^ xor_coef) + 1) << (32 - idx) >> scale_shift;
                    (*smpl_rw)[ch][i][sb] =
                        MUL32_MP3_32S(smpl, scale_mul);
                }
            } else {
                for (i = 0; i < 12; i++) {
                    (*smpl_rw)[ch][i][sb] = 0;
                }
            }
        }  // for sb

        for (i = 0; i < 12; i++) {
            ippsSynthPQMF_MP3_32s16s_x((Ipp32s *)&((*(state->smpl_rw))[ch][i][0]),
                m_pOutSamples + 32 * stereo * i + ch,
                state->synth_buf + IPP_MP3_V_BUF_LEN * ch,
                &(state->synth_ind[ch]),
                stereo);
        }
    }    // for ch

    return 1;
}
