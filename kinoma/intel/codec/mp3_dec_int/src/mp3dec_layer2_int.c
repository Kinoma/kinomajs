/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

//***
#include "kinoma_ipp_lib.h"

#include "mp3dec_own_int.h"

/* Q30 */
static int C_quant[] = {
     1431655808,
     1717986944,
     1227133568,
     1908874368,
     1145324672,
     1108378624,
     1090785408,
     1082196480,
     1077952640,
     1075843072,
     1074791424,
     1074266368,
     1074004096,
     1073872896,
     1073807360,
     1073774592,
     1073758208,
};

static int D_quant[] = {
   1073741824,
   1073741824,
    536870912,
   1073741824,
    268435456,
    134217728,
     67108864,
     33554432,
     16777216,
      8388608,
      4194304,
      2097152,
      1048576,
       524288,
       262144,
       131072,
        65536,
    };

/* Q28 */
static int scale_values_int[3] = {
      268435456,
      213057360,
      169103744,
};

int mp3dec_decode_data_LayerII(MP3Dec *state)
{
    int i, ch, sb;
    int           (*sample)[32][36] = state->com.sample;
    int           sblimit = state->com.sblimit;
    short         (*scalefactor)[3][32] = state->com.scalefactor;
    short (*allocation)[32] = state->com.allocation;
    sampleintrw *smpl_rw = state->smpl_rw;     // out of imdct
    int stereo = state->com.stereo;
    short  *m_pOutSamples = state->com.m_pOutSamples;
    unsigned char *alloc_table = state->com.alloc_table;
    int scale_fact, scale_mul, scale_shift, scale_mod;

    for (ch = 0; ch < stereo; ch++) {
        for (sb = 0; sb < sblimit; sb++) {
            if (allocation[ch][sb] != 0) {
                int idx = alloc_table[sb * 16 + allocation[ch][sb]];
                int x = mp3dec_numbits[idx];
                int xor_coef;
                int *sample_ptr = &sample[ch][sb][0];

                xor_coef = (1 << (x - 1));
                scale_fact = scalefactor[ch][0][sb];
                scale_shift = scale_fact / 3;
                scale_mod = scale_fact - scale_shift * 3;
                scale_mul = MUL32_MP3_32S(scale_values_int[scale_mod],
                    C_quant[idx]);
                for (i = 0; i < 12; i++) {
                    int smpl = (sample_ptr[i] ^ xor_coef) << (32 - x);

                    // Dequantize the sample
                    smpl += D_quant[idx];
                    smpl >>= scale_shift;
                    (*smpl_rw)[ch][i][sb] =
                        MUL32_MP3_32S(smpl, scale_mul);
                }

                scale_fact = scalefactor[ch][1][sb];
                scale_shift = scale_fact / 3;
                scale_mod = scale_fact - scale_shift * 3;
                scale_mul = MUL32_MP3_32S(scale_values_int[scale_mod],
                    C_quant[idx]);
                for (i = 12; i < 18; i++) {
                    int smpl = (sample_ptr[i] ^ xor_coef) << (32 - x);

                    // Dequantize the sample
                    smpl += D_quant[idx];
                    smpl >>= scale_shift;
                    (*smpl_rw)[ch][i][sb] =
                        MUL32_MP3_32S(smpl, scale_mul);
                }
            } else {
                for (i = 0; i < 18; i++) {
                    (*smpl_rw)[ch][i][sb] = 0;
                }
            }
        }  // for sb

        for (sb = sblimit; sb < 32; sb++) {
            for (i = 0; i < 18; i++) {
                (*smpl_rw)[ch][i][sb] = 0;
            }
        }

        for (i = 0; i < 18; i++) {
            ippsSynthPQMF_MP3_32s16s_x((Ipp32s *)&((*(state->smpl_rw))[ch][i][0]),
                m_pOutSamples + 32 * stereo * i + ch,
                state->synth_buf + IPP_MP3_V_BUF_LEN * ch,
                &(state->synth_ind[ch]),
                stereo);
        }
    }    // for ch

    for (ch = 0; ch < stereo; ch++) {
        for (sb = 0; sb < sblimit; sb++) {
            if (allocation[ch][sb] != 0) {
                int idx = alloc_table[sb * 16 + allocation[ch][sb]];
                int x = mp3dec_numbits[idx];
                int xor_coef;
                int *sample_ptr = &sample[ch][sb][18];

                xor_coef = (1 << (x - 1));

                scale_fact = scalefactor[ch][1][sb];
                scale_shift = scale_fact / 3;
                scale_mod = scale_fact - scale_shift * 3;
                scale_mul = MUL32_MP3_32S(scale_values_int[scale_mod],
                    C_quant[idx]);
                for (i = 0; i < 6; i++) {
                    int smpl = (sample_ptr[i] ^ xor_coef) << (32 - x);

                    // Dequantize the sample
                    smpl += D_quant[idx];
                    smpl >>= scale_shift;
                    (*smpl_rw)[ch][i][sb] =
                        MUL32_MP3_32S(smpl, scale_mul);
                }

                scale_fact = scalefactor[ch][2][sb];
                scale_shift = scale_fact / 3;
                scale_mod = scale_fact - scale_shift * 3;
                scale_mul = MUL32_MP3_32S(scale_values_int[scale_mod],
                    C_quant[idx]);
                for (i = 6; i < 18; i++) {
                    int smpl = (sample_ptr[i] ^ xor_coef) << (32 - x);

                    // Dequantize the sample
                    smpl += D_quant[idx];
                    smpl >>= scale_shift;
                    (*smpl_rw)[ch][i][sb] =
                        MUL32_MP3_32S(smpl, scale_mul);
                }
            } else {
                for (i = 0; i < 18; i++) {
                    (*smpl_rw)[ch][i][sb] = 0;
                }
            }
        }  // for sb

        for (sb = sblimit; sb < 32; sb++) {
            for (i = 0; i < 18; i++) {
                (*smpl_rw)[ch][i][sb] = 0;
            }
        }

        for (i = 0; i < 18; i++) {
            ippsSynthPQMF_MP3_32s16s_x((Ipp32s *)&((*(state->smpl_rw))[ch][i][0]),
                m_pOutSamples + 576 * stereo + 32 * stereo * i + ch,
                state->synth_buf + IPP_MP3_V_BUF_LEN * ch,
                &(state->synth_ind[ch]),
                stereo);
        }
    }    // for ch

    return 1;
}
