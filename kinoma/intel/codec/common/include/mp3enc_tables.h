/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3ENC_TABLES_H__
#define __MP3ENC_TABLES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _scalefac_struct
{
  unsigned int l[23];
  unsigned int s[14];
} scalefac_struct;

extern scalefac_struct      mp3enc_sfBandIndex[3];

extern const int mp3enc_frequency[3];
extern const int mp3enc_bitrate[15];

typedef short      IXS[192][3];

extern int        mp3enc_slen1_tab[16];
extern int        mp3enc_slen2_tab[16];
extern int        mp3enc_scfsi_band_long[5];
extern int        mp3enc_pretab[21];

extern int        mp3enc_region01_table[23][2];

#ifdef __cplusplus
}
#endif

#endif  //  __MP3ENC_TABLES_H__
