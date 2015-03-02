/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SBRDEC_H__
#define __SBRDEC_H__

#include "ippac.h"
#include "ipps.h"
#include "aaccmn_chmap.h"

/* my val */
#define SBR_NOT_SCALABLE   0
#define SBR_MONO_BASE      1
#define SBR_STEREO_ENHANCE 2
#define SBR_STEREO_BASE    3
#define SBR_NO_DATA        4

/* standard */
#define FIXFIX 0
#define FIXVAR 1
#define VARFIX 2
#define VARVAR 3

/* standard */
#define NUM_TIME_SLOTS     16
#define SBR_DEQUANT_OFFSET 6
#define RATE               2

#define SBR_TIME_HFGEN     8
#define SBR_TIME_HFADJ     2

/* SBR CONST */
#define MAX_SIZE_FREQ_TBLS 64
#define MAX_NUM_PATCHES     6
#define MAX_NUM_ENV         5
#define MAX_NUM_ENV_VAL   100
#define MAX_NUM_NOISE_VAL  5

#define HEAAC_HQ_MODE      18
#define HEAAC_LP_MODE      28
#define HEAAC_DWNSMPL_ON    1
#define HEAAC_DWNSMPL_OFF   0
#define HEAAC_PARAMS_UNDEF  -999

/* SBR_ERR_PARSER */
#define SBR_ERR_REQUIREMENTS -13

/* SIZE OF WORK BUFFER */
#define SBR_MINSIZE_OF_WORK_BUFFER (6 * 100 * sizeof(Ipp32s))


typedef struct {

/* SBR header */
  Ipp32s  bs_amp_res;

  Ipp32s  bs_start_freq;
  Ipp32s  bs_stop_freq;
  Ipp32s  bs_freq_scale;
  Ipp32s  bs_alter_scale;
  Ipp32s  bs_xover_band;
  Ipp32s  bs_noise_bands;

  Ipp32s  bs_limiter_bands;
  Ipp32s  bs_limiter_gains;
  Ipp32s  bs_interpol_freq;
  Ipp32s  bs_smoothing_mode;

  Ipp32s  Reset;

  /* SBR bitstream & data element */
  Ipp32s  bs_add_harmonic_flag[2];
  Ipp32s  bs_add_harmonic_flag_prev[2];
  Ipp32s  bs_add_harmonic[2][MAX_NUM_ENV_VAL];
  Ipp32s  bs_add_harmonic_prev[2][MAX_NUM_ENV_VAL];

  Ipp32s  bs_sbr_crc_bits;
  Ipp32s  bs_extended_data;
  Ipp32s  bs_extension_size;
  Ipp32s  bs_extension_id;

  Ipp32s  bs_coupling;
  Ipp32s  bs_frame_class[2];
  Ipp32s  bs_pointer[2];
  Ipp32s  bs_df_env[2][MAX_NUM_ENV];
  Ipp32s  bs_df_noise[2][MAX_NUM_ENV];
  Ipp32s  bs_invf_mode[2][MAX_NUM_NOISE_VAL];
  Ipp32s  bs_invf_mode_prev[2][MAX_NUM_NOISE_VAL];

  Ipp32s vSizeEnv[2][5+1];
  Ipp16s vecEnv[2][5*64];
  Ipp16s vecEnvPrev[2][MAX_NUM_ENV_VAL];

  Ipp32s vSizeNoise[2][2+1];
  Ipp16s vecNoise[2][2*5];
  Ipp16s vecNoisePrev[2][MAX_NUM_ENV_VAL];

/* all sbr tables */

  /* Huffman's tables (from core AAC dec) */
  void   *sbrHuffTables[10];

  /* freq tables */
  Ipp32s  f_master[MAX_SIZE_FREQ_TBLS];
  Ipp32s  f_TableHigh[MAX_SIZE_FREQ_TBLS];
  Ipp32s  f_TableLow[MAX_SIZE_FREQ_TBLS];
  Ipp32s  f_TableNoise[MAX_SIZE_FREQ_TBLS];
  Ipp32s  f_TableLim[MAX_SIZE_FREQ_TBLS];

  Ipp32s  patchNumSubbands[MAX_NUM_PATCHES];
  Ipp32s  patchStartSubband[MAX_NUM_PATCHES];

  /* size of freq tables */
  Ipp32s  numPatches;
  Ipp32s  N_master;
  Ipp32s  N_high;
  Ipp32s  N_low;
  Ipp32s  N_L;
  Ipp32s  N_Q;
  Ipp32s  N_Q_prev;

  /* boundary band */
  Ipp32s  M;
  Ipp32s  M_prev;
  Ipp32s  kx;
  Ipp32s  kx_prev;
  Ipp32s  k0;

/* TF Grid */
  Ipp32s  tE[2][MAX_NUM_ENV_VAL];

  Ipp32s  tQ[2][MAX_NUM_ENV_VAL];
  Ipp32s  L_Q[2];
  Ipp32s  L_E[2];
  Ipp32s  L_Q_prev[2];
  Ipp32s  L_E_prev[2];
  Ipp16s  r[2][MAX_NUM_ENV];
  Ipp16s  r_prev[2][MAX_NUM_ENV];

/* for HF Adjusment */
  Ipp32s  S_index_mapped_prev[2][MAX_NUM_ENV_VAL];

  Ipp32s  lA_prev[2];

  Ipp32s  indexNoise[2];
  Ipp32s  indexSine[2];

  Ipp32s  FlagUpdate[2];

  /* last step band */
  Ipp32s  transitionBand[2];

/* core AAC */
  Ipp32s  SbrLayer;
  Ipp32s  id_aac;
  Ipp32s  FreqSbr;
  Ipp32s  sbr_freq_indx;

/* for APPLICATION */
  Ipp32s  sbrHeaderFlagPresent;

/* for DEBUG */
  Ipp32s  sbrFlagError;
  Ipp32s  cnt_bit;

} sSbrDecCommon;   /* common SBR structure */

#ifdef  __cplusplus
extern  "C" {
#endif

  Ipp32s sbrdecResetCommon( sSbrDecCommon* pSbr );

#ifdef  __cplusplus
}
#endif

#endif             /* __SBRDEC_H__ */
/* EOF */
