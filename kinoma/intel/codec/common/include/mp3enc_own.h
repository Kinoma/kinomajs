/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MP3ENC_OWN_H__
#define __MP3ENC_OWN_H__

#include "ippac.h"
#include "ippdc.h"
#include "ipps.h"

#include "mp3enc.h"
#include "mp3enc_tables.h"
#include "mp3enc_hufftables.h"
#include "mp3enc_psychoacoustic.h"
#include "bstream.h"

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SI_MAX 10

typedef struct {
    void    *phuftable;
    short   mav_value;
    short   linbits;
} MP3Enc_HuffmanTable;

typedef struct {
//  Quantization
    VM_ALIGN16_DECL(short) quant_ix[2][2][576];

    VM_ALIGN16_DECL(unsigned short) scalefac_l[2][2][32];
    VM_ALIGN16_DECL(unsigned short) scalefac_s[2][2][12][3];
    int     max_bits[2][2];   // max bit for encode granule

//  huffman tables
    MP3Enc_HuffmanTable htables[32];

    IppMP3FrameHeader header;

// wav parameters
    int     stereo;
    int     frameSize;

// SIDE INFO
    unsigned int si_main_data_begin;
    unsigned int si_private_bits;
    unsigned int si_part23Len[2][2];
    unsigned int si_bigVals[2][2];
    unsigned int si_count1[2][2];

    short   si_globGain[2][2];
    unsigned int si_sfCompress[2][2];
    unsigned int si_winSwitch[2][2];
    unsigned int si_blockType[2][2];
    unsigned int si_mixedBlock[2][2];
    unsigned int si_pTableSelect[2][2][3];
    short   si_pSubBlkGain[2][2][3];
    unsigned int si_address[2][2][3];
    unsigned int si_reg0Cnt[2][2];
    unsigned int si_reg1Cnt[2][2];
    unsigned int si_preFlag[2][2];
    unsigned int si_sfScale[2][2];
    unsigned int si_cnt1TabSel[2][2];
    unsigned int si_scfsi[2][4];
    unsigned int si_part2Len[2][2];

    unsigned int sfb_lmax, sfb_smax;

// END OF SIDE INFO

    sBitsreamBuffer mainStream;
    sBitsreamBuffer sideStream;

    unsigned int     buffer_main_data[1024];
    unsigned int     buffer_side_info[256];

    int     slot_size;
    int     main_data_ptr;
    int     resr_bytes;
    int     resr_mod_slot;

    int     framesNum;


    int quant_mode_fast;

    int lowpass_maxline;

    unsigned char si_buf[SI_MAX][40];
    int si_beg, si_new, si_num;
} MP3Enc_com;

int     mp3enc_huffInit(MP3Enc_HuffmanTable *htables);
void    mp3enc_huffFree(MP3Enc_HuffmanTable *htables);
void    mp3enc_huffCodeBits(MP3Enc_com *state, int gr, int ch);

void    mp3enc_encodeBigValues(MP3Enc_com *state, short *pInput, int gr, int ch);
int     mp3enc_encodeSideInfo(MP3Enc_com *state);
void    mp3enc_encodeMainData(MP3Enc_com *state);

int     mp3enc_writeFrame(MP3Enc_com *state, int si_bits, int bits, unsigned char *pOutputData);
int     mp3enc_formatBitstream(MP3Enc_com *state, int (*mdct_out)[2][576], unsigned char *pOutputData);

int     mp3enc_quantChooseTableLong(MP3Enc_com *state, int gr, int ch, short *pInput, int length,
                        int table);
int     mp3enc_quantCalcBitsLong(MP3Enc_com *state, short *pInput, int gr, int ch);
int     mp3enc_quantCalcBits(MP3Enc_com *state, int gr, int ch, short *ipptmp0_576);

void    mp3enc_quantIterReset(MP3Enc_com *state, int gr, int ch);
int     mp3enc_quantCalcPart2Len(MP3Enc_com *state, int gr, int ch);
int     mp3enc_quantScaleBitCount(MP3Enc_com *state, int gr, int ch);

#ifdef __cplusplus
}
#endif

#endif
