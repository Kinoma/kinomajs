/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3DEC_OWN_FP_H__
#define __MP3DEC_OWN_FP_H__

#include "math.h"

#include "mp3dec_own.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int sampleint[2][576];
typedef int sampleintrw[2][18][32];

struct _MP3Dec {
    MP3Dec_com com;
    VM_ALIGN16_DECL(unsigned int) global[2880];
    VM_ALIGN16_DECL(int) prevblk[2][576];
    float tmp_bfr[32];
    int filter_bfr[2][16][64];

    sampleint *smpl_xr;       // out of dequantizer
    sampleint *smpl_ro;       // out of reordering
    sampleint *smpl_re;       // out of antialiasing
    sampleintrw *smpl_rw;     // out of imdct
    sampleshort *smpl_sb;       // out of subband synth

    int GlobalScaleFactor_m[2][2];
    int GlobalScaleFactor_e[2][2];

//    IppsMDCTInvSpec_32f *pMDCTSpecShort;
//    IppsMDCTInvSpec_32f *pMDCTSpecLong;
//    IppsDCTFwdSpec_32f  *pDCTSpecFB;

//    unsigned char *mdct_buffer;

    VM_ALIGN16_DECL(Ipp32s) synth_buf[IPP_MP3_V_BUF_LEN*2];
    Ipp32s synth_ind[2];

    int dctnum_prev[2];

    short   m_ptr[2][2];
    short   m_even[2];
};

int mp3dec_SubBandSynth(MP3Dec *state, int ch, int num);
int mp3dec_decode_data_LayerI(MP3Dec *state);
int mp3dec_decode_data_LayerII(MP3Dec *state);
MP3Status mp3dec_decode_data_LayerIII(MP3Dec *state);

extern int mp3dec_Dcoef[64][15];

#define MUL32_MP3_32S(x, y) \
  (Ipp32s)(((Ipp64s)((Ipp64s)(x) * (Ipp64s)(y)))>>32)

// #define FMUL32_MP3_32S(x, y) MUL32_MP3_32S((x) << 1, y)

#define MUL32_MP3_64S(x, y) \
  (Ipp64s)(((Ipp64s)((Ipp64s)(x) * (Ipp64s)(y))))
#define MUL32_MP3_32S64S(x, y) \
  (((Ipp64s)((Ipp64s)(x) * (Ipp64s)(y))) >> 32)

#ifdef __cplusplus
}
#endif

#endif
