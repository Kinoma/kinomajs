/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3DEC_OWN_H__
#define __MP3DEC_OWN_H__

#include "ippac.h"
#include "ippdc.h"
#include "ipps.h"

#include "mp3dec.h"
#include "mp3dec_alloc_tab.h"
#include "mp3dec_huftabs.h"
#include "bstream.h"

#include "vm_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef short sampleshort[2][576];

typedef struct {
    short   l[23];    /* [sb] */
    short   s[3][13]; /* [window][sb] */
} sScaleFactors;

typedef struct
{
    int     linbits;
    void   *phuftable;
} sHuffmanTable;

typedef struct {
    IppMP3FrameHeader header;
    IppMP3FrameHeader header_good;
    int     mpg25, mpg25_good;

    int     stereo;
    int     intensity;
    int     ms_stereo;
    int     MP3nSlots;

/*
 * SIDE INFO
 */
    unsigned int si_main_data_begin;
    unsigned int si_private_bits;
    unsigned int si_part23Len[2][2];
    unsigned int si_bigVals[2][2];
    short   si_globGain[2][2];
    unsigned int si_sfCompress[2][2];
    unsigned int si_winSwitch[2][2];
    unsigned int si_blockType[2][2];
    unsigned int si_mixedBlock[2][2];
    unsigned int si_pTableSelect[2][2][3];
    short   si_pSubBlkGain[2][2][3];
    unsigned int si_reg0Cnt[2][2];
    unsigned int si_reg1Cnt[2][2];
    unsigned int si_preFlag[2][2];
    unsigned int si_sfScale[2][2];
    unsigned int si_cnt1TabSel[2][2];
    unsigned int si_scfsi[2];
    int s_len[4];
    int blocknumber;
    int blocktypenumber;
/*
 * END OF SIDE INFO
 */

    sampleshort *smpl_xs;       // out of huffman

    sScaleFactors ScaleFactors[2];
#ifndef KINOMA_FAST_HUFFMAN
    sHuffmanTable huff_table[34];
#endif
    int     part2_start;

    sBitsreamBuffer m_StreamData;
    sBitsreamBuffer m_MainData;

    int    non_zero[2];
    short  *m_pOutSamples;

    int     decodedBytes;

    int     MAINDATASIZE;

    short         allocation[2][32];
    short         scfsi[2][32];
    short         scalefactor[2][3][32];
    short         scalefactor_l1[2][32];
    int           sample[2][32][36];
    int           *nbal_alloc_table;
    unsigned char *alloc_table;
    int           jsbound;
    int           sblimit;

    int     m_layer;

    int     m_nBitrate;
    int     m_frame_num;
    int     m_bInit;

    int     id3_size;

} MP3Dec_com;

typedef struct {
    int     l[23];
    int     s[14];
} ssfBandIndex;

extern int mp3dec_bitrate[2][3][15];
extern int mp3dec_frequency[3][4];
extern unsigned char mp3dec_nr_of_sfb[6][3][4];
extern ssfBandIndex mp3dec_sfBandIndex[3][3];

MP3Status mp3decInit_com(MP3Dec_com *state_ptr);
MP3Status mp3decClose_com(MP3Dec_com *state);
MP3Status mp3decReset_com(MP3Dec_com *state);

int mp3dec_audio_data_LayerI(MP3Dec_com *state);
int mp3dec_audio_data_LayerII(MP3Dec_com *state);

int mp3dec_initialize_huffman(sHuffmanTable *huff_table);

int mp3dec_Huffmancodebits(MP3Dec_com *state, int gr, int ch);

int mp3dec_GetScaleFactorsL3(MP3Dec_com *state, int gr, int ch);
int mp3dec_GetScaleFactorsL3_LSF(MP3Dec_com *state, int ch);

int mp3dec_audio_data_LayerIII(MP3Dec_com *state);
int mp3dec_audio_data_LSF_LayerIII(MP3Dec_com *state);

int mp3dec_SetAllocTable(MP3Dec_com *state);
int mp3dec_ReadMainData(MP3Dec_com *state);
MP3Status  mp3dec_GetSynch(MP3Dec_com *state);
MP3Status mp3dec_ReceiveBuffer(sBitsreamBuffer *m_StreamData, void *in_GetPointer, int in_GetDataSize);
MP3Status mp3dec_GetID3Len(Ipp8u *in, int inDataSize, MP3Dec_com *state);
MP3Status mp3dec_SkipID3(int inDataSize, int *skipped, MP3Dec_com *state);

#ifdef __cplusplus
}
#endif

#endif
