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
#include "kinoma_dec_huff.h"

#include "mp3dec_own.h"

/* scfsi_band  scalefactor bands (p.24 ISO/IEC 11172-3 )*/
static const struct {
    int     l[5];
    int     s[3];
} sfbtable = {
    {
        0, 6, 11, 16, 21}, {
            0, 6, 12}
};

static const int slen[2][16] = {
    {0x0, 0x0, 0x0, 0x0, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x2, 0x3, 0x3, 0x3, 0x4, 0x4},
    {0x0, 0x1, 0x2, 0x3, 0x0, 0x1, 0x2, 0x3, 0x1, 0x2, 0x3, 0x1, 0x2, 0x3, 0x2, 0x3}
};

/* Table 3-B.8. Layer III scalefactor bands (p.33 Annex_AB ISO/IEC 11172-3)*/

const ssfBandIndex mp3dec_sfBandIndex[3][3] = {
    {
        {
            {
                0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
                    284, 336, 396, 464, 522, 576}, {
                        0, 4, 8, 12, 18, 24, 32, 42, 56, 74, 100, 132, 174, 192}
        }, {
            {
                0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 114, 136, 162, 194, 232,
                    278, 330, 394, 464, 540, 576}, {
                        0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 136, 180, 192}
        }, {
            {
                0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
                    284, 336, 396, 464, 522, 576}, {
                        0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192}
        },},
    {
            {
                {
                    0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90, 110, 134, 162,
                        196, 238, 288, 342, 418, 576}, {
                            0, 4, 8, 12, 16, 22, 30, 40, 52, 66, 84, 106, 136, 192}
            },        /* Table 3-B.8b */
            {
                {
                    0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88, 106, 128, 156,
                        190, 230, 276, 330, 384, 576}, {
                            0, 4, 8, 12, 16, 22, 28, 38, 50, 64, 80, 100, 126, 192}
            },        /* Table 3-B.8c */
            {
                {
                    0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82, 102, 126, 156, 194,
                        240, 296, 364, 448, 550, 576}, {
                            0, 4, 8, 12, 16, 22, 30, 42, 58, 78, 104, 138, 180, 192}
            } /* Table 3-B.8a */
        },
        {
            {
                {
                   0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
                       284, 336, 396, 464, 522, 576}, {
                         0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192}
            },
            {
                {
                   0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
                        284, 336, 396, 464, 522, 576}, {
                          0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192}
            },
            {
              {0, 12, 24, 36, 48, 60, 72, 88, 108, 132, 160, 192, 232, 280, 336, 400, 476,
                   566, 568, 570, 572, 574, 576}, {
                     0,  8,  16,  24, 36, 52, 72, 96, 124, 160, 162, 164, 166, 192}
            },
        }
};


const unsigned char mp3dec_nr_of_sfb[6][3][4] =
{
    {{0x6, 0x5, 0x5, 0x5}, { 0x9,  0x9, 0x9, 0x9}, {0x6,  0x9, 0x9, 0x9}},
    {{0x6, 0x5, 0x7, 0x3}, { 0x9,  0x9, 0xc, 0x6}, {0x6,  0x9, 0xc, 0x6}},
    {{0xb, 0xa, 0x0, 0x0}, {0x12, 0x12, 0x0, 0x0}, {0xf, 0x12, 0x0, 0x0}},
    {{0x7, 0x7, 0x7, 0x0}, { 0xc,  0xc, 0xc, 0x0}, {0x6,  0xf, 0xc, 0x0}},
    {{0x6, 0x6, 0x6, 0x3}, { 0xc,  0x9, 0x9, 0x6}, {0x6,  0xc, 0x9, 0x6}},
    {{0x8, 0x8, 0x5, 0x0}, { 0xf,  0xc, 0x9, 0x0}, {0x6, 0x12, 0x9, 0x0}}
};

/******************************************************************************
//  Name:
//    GetScaleFactors
//
//  Description:
//    read scalefactors for one granule of channel
//
//  Input Arguments:
//    state - point to MP3Dec structure
//    gr - number of granule
//    ch - number of channel
//
//  Output Arguments:
//    ScaleFactors - array of scalefactors
//
//  Returns:
//    1 - all ok
//
******************************************************************************/
int mp3dec_GetScaleFactorsL3(MP3Dec_com *state, int gr, int ch) {
    int     sfb, window, i;
    sBitsreamBuffer *BS = &(state->m_MainData);
    sScaleFactors *ScaleFactors = state->ScaleFactors;
    unsigned int (*si_blockType)[2] = state->si_blockType;
    unsigned int (*si_mixedBlock)[2] = state->si_mixedBlock;
    unsigned int *si_scfsi = state->si_scfsi;
    unsigned int (*si_sfCompress)[2] = state->si_sfCompress;
    unsigned int (*si_winSwitch)[2] = state->si_winSwitch;

    if (si_winSwitch[gr][ch] && (si_blockType[gr][ch] == 2)) {
        if (si_mixedBlock[gr][ch]) {
            for (sfb = 0; sfb < 8; sfb++)
                if (slen[0][si_sfCompress[gr][ch]] != 0)
                    GET_BITS(BS, ScaleFactors[ch].l[sfb],
                        slen[0][si_sfCompress[gr][ch]])
                else
                    ScaleFactors[ch].l[sfb] = 0;
            for (sfb = 3; sfb < 6; sfb++)
                for (window = 0; window < 3; window++)
                    if (slen[0][si_sfCompress[gr][ch]] != 0)
                        GET_BITS(BS, ScaleFactors[ch].s[window][sfb],
                            slen[0][si_sfCompress[gr][ch]])
                    else
                        ScaleFactors[ch].s[window][sfb] = 0;

            for (sfb = 6; sfb < 12; sfb++)
                for (window = 0; window < 3; window++)
                    if (slen[1][si_sfCompress[gr][ch]] != 0)
                        GET_BITS(BS, ScaleFactors[ch].s[window][sfb],
                            slen[1][si_sfCompress[gr][ch]])
                    else
                        ScaleFactors[ch].s[window][sfb] = 0;
        } else {  /* SHORT */
            for (i = 0; i < 2; i++)
                for (sfb = sfbtable.s[i]; sfb < sfbtable.s[i + 1]; sfb++)
                    for (window = 0; window < 3; window++)
                        if (slen[i][si_sfCompress[gr][ch]] != 0)
                            GET_BITS(BS, ScaleFactors[ch].s[window][sfb],
                                slen[i][si_sfCompress[gr][ch]])
                        else
                            ScaleFactors[ch].s[window][sfb] = 0;
        }
        ScaleFactors[ch].s[0][12] = 0;
        ScaleFactors[ch].s[1][12] = 0;
        ScaleFactors[ch].s[2][12] = 0;
    } else {    /* LONG types 0,1,3 */
        for (i = 0; i < 4; i++) {
            if ((((si_scfsi[ch] >> (3 - i)) & 1) == 0) || (gr == 0)) {
                for (sfb = sfbtable.l[i]; sfb < sfbtable.l[i + 1]; sfb++) {
                    if (slen[(i < 2) ? 0 : 1][si_sfCompress[gr][ch]] != 0) {
                        GET_BITS(BS, ScaleFactors[ch].l[sfb],
                            slen[(i < 2) ? 0 : 1][si_sfCompress[gr][ch]]);
                    } else {
                        ScaleFactors[ch].l[sfb] = 0;
                    }
                }
            }
        }
    }

    return 1;
}

/******************************************************************************
//  Name:
//    audio_data_LayerIII
//
//  Description:
//    read side information
//
//  Input Arguments:
//    state - point to MP3Dec structure
//
//  Output Arguments:
//    state - point to MP3Dec structure, side info
//
//  Returns:
//    1 - all ok
//   -2 - wrong block type
//
******************************************************************************/
int mp3dec_audio_data_LayerIII(MP3Dec_com *state) {
    int     gr, ch;
    int     tmp;

    sBitsreamBuffer *BS = &(state->m_StreamData);

    unsigned int si_main_data_begin;
    unsigned int si_private_bits;
    unsigned int (*si_part23Len)[2] = state->si_part23Len;
    unsigned int (*si_bigVals)[2] = state->si_bigVals;
    short   (*si_globGain)[2] = state->si_globGain;
    unsigned int (*si_sfCompress)[2] = state->si_sfCompress;
    unsigned int (*si_winSwitch)[2] = state->si_winSwitch;
    unsigned int (*si_blockType)[2] = state->si_blockType;
    unsigned int (*si_mixedBlock)[2] = state->si_mixedBlock;
    unsigned int (*si_pTableSelect)[2][3] = state->si_pTableSelect;
    short   (*si_pSubBlkGain)[2][3] = state->si_pSubBlkGain;
    unsigned int (*si_reg0Cnt)[2] = state->si_reg0Cnt;
    unsigned int (*si_reg1Cnt)[2] = state->si_reg1Cnt;
    unsigned int (*si_preFlag)[2] = state->si_preFlag;
    unsigned int (*si_sfScale)[2] = state->si_sfScale;
    unsigned int (*si_cnt1TabSel)[2] = state->si_cnt1TabSel;
    unsigned int *si_scfsi = state->si_scfsi;

    int stereo = state->stereo;

    GET_BITS(BS, si_main_data_begin, 9);

    if (stereo == 1)
        GET_BITS(BS, si_private_bits, 5)
    else
        GET_BITS(BS, si_private_bits, 3)

    state->si_main_data_begin = si_main_data_begin;
    state->si_private_bits = si_private_bits;

    for (ch = 0; ch < stereo; ch++)
        GET_BITS(BS, si_scfsi[ch], 4);

    for (gr = 0; gr < 2; gr++) {
        for (ch = 0; ch < stereo; ch++) {
            GET_BITS(BS, tmp, 12 + 9);
            si_part23Len[gr][ch] = tmp >> 9;
            si_bigVals[gr][ch] = tmp & 0x01ff;

            GET_BITS(BS, tmp, 8 + 4 + 1);
            si_globGain[gr][ch] = (short)(tmp >> 5);
            si_sfCompress[gr][ch] = (tmp >> 1) & 0x0f;
            si_winSwitch[gr][ch] = tmp & 1;

            GET_BITS(BS, tmp, 22);
            if (si_winSwitch[gr][ch]) {
                si_blockType[gr][ch] = tmp >> 20;
                si_mixedBlock[gr][ch] = (tmp >> 19) & 1;
                si_pTableSelect[gr][ch][0] = (tmp >> 14) & 0x01f;
                si_pTableSelect[gr][ch][1] = (tmp >> 9) & 0x01f;
                si_pSubBlkGain[gr][ch][0] = (short)(-8 * (int)((tmp >> 6) & 0x07));
                si_pSubBlkGain[gr][ch][1] = (short)(-8 * (int)((tmp >> 3) & 0x07));
                si_pSubBlkGain[gr][ch][2] = (short)(-8 * (int)(tmp & 0x07));

                if (si_blockType[gr][ch] == 0)
                    return -2;  // wrong block type

                si_reg0Cnt[gr][ch] = 7 + (si_blockType[gr][ch] == 2 &&
                    si_mixedBlock[gr][ch] == 0);
                si_reg1Cnt[gr][ch] = 20 - si_reg0Cnt[gr][ch];
            } else {
                si_pTableSelect[gr][ch][0] = tmp >> 17;
                si_pTableSelect[gr][ch][1] = (tmp >> 12) & 0x01f;
                si_pTableSelect[gr][ch][2] = (tmp >> 7) & 0x01f;
                si_reg0Cnt[gr][ch] = (tmp >> 3) & 0x0f;
                si_reg1Cnt[gr][ch] = tmp & 7;

                si_blockType[gr][ch] = 0;
            }

            GET_BITS(BS, tmp, 3);
            si_preFlag[gr][ch] = tmp >> 2;
            si_sfScale[gr][ch] = (tmp >> 1) & 1;
            si_cnt1TabSel[gr][ch] = tmp & 1;
        }
    }

    return 1;
}

/******************************************************************************
//  Name:
//    initialize_huffman
//
//  Description:
//    initialize huffman tables by using ippAC functionality
//
//  Input Arguments:
//    huff_table - point to array of Huff_table structures.
//    huff_table[i]->ptable is the table in user format.
//
//  Output Arguments:
//    huff_table - point to array of Huff_table structures.
//    huff_table[i]->phuftable is the table in internal format.
//
//  Returns:
//    -
//
******************************************************************************/
#ifndef KINOMA_FAST_HUFFMAN
int mp3dec_initialize_huffman(sHuffmanTable *huff_table)
{
    int     i;

    huff_table[16].linbits = 1;
    huff_table[17].linbits = 2;
    huff_table[18].linbits = 3;
    huff_table[19].linbits = 4;
    huff_table[20].linbits = 6;
    huff_table[21].linbits = 8;
    huff_table[22].linbits = 10;
    huff_table[23].linbits = 13;
    huff_table[24].linbits = 4;
    huff_table[25].linbits = 5;
    huff_table[26].linbits = 6;
    huff_table[27].linbits = 7;
    huff_table[28].linbits = 8;
    huff_table[29].linbits = 9;
    huff_table[30].linbits = 11;
    huff_table[31].linbits = 13;

    for (i = 1; i < 16; i++) {
        if (mp3dec_VLCBooks[i]) {
            ippsVLCDecodeInitAlloc_32s_x(mp3dec_VLCBooks[i], mp3dec_VLCTableSizes[i],
                mp3dec_VLCSubTablesSizes[i],
                mp3dec_VLCNumSubTables[i],
                (IppsVLCDecodeSpec_32s **) (&huff_table[i].
                phuftable));
        }
    }

    ippsVLCDecodeInitAlloc_32s_x(mp3dec_VLCBooks[16], mp3dec_VLCTableSizes[16],
        mp3dec_VLCSubTablesSizes[16], mp3dec_VLCNumSubTables[16],
        (IppsVLCDecodeSpec_32s **) (&huff_table[16].
        phuftable));

    for (i = 17; i < 24; i++) {
        huff_table[i].phuftable = huff_table[16].phuftable;
    }

    ippsVLCDecodeInitAlloc_32s_x(mp3dec_VLCBooks[24], mp3dec_VLCTableSizes[24],
        mp3dec_VLCSubTablesSizes[24], mp3dec_VLCNumSubTables[24],
        (IppsVLCDecodeSpec_32s **) (&huff_table[24].
        phuftable));
    for (i = 25; i < 32; i++) {
        huff_table[i].phuftable = huff_table[24].phuftable;
    }

    ippsVLCDecodeInitAlloc_32s_x(mp3dec_VLCBooks[32], mp3dec_VLCTableSizes[32],
        mp3dec_VLCSubTablesSizes[32], mp3dec_VLCNumSubTables[32],
        (IppsVLCDecodeSpec_32s **) (&huff_table[32].
        phuftable));

    ippsVLCDecodeInitAlloc_32s_x(mp3dec_VLCBooks[33], mp3dec_VLCTableSizes[33],
        mp3dec_VLCSubTablesSizes[33], mp3dec_VLCNumSubTables[33],
        (IppsVLCDecodeSpec_32s **) (&huff_table[33].
        phuftable));
    return 1;
}
#endif

/******************************************************************************
//  Name:
//    Huffmancodebits
//
//  Description:
//    Huffman decoding of spectral samples
//
//  Input Arguments:
//    DC - point to sDecoderContext structure
//    gr - number of granule
//    ch - number of channel
//
//  Output Arguments:
//    smpl_xs[ch] - array of decoded samples
//
//  Returns:
//    1 - all ok
//
******************************************************************************/
int mp3dec_Huffmancodebits(MP3Dec_com *state, int gr, int ch) {
    int     j, i, idx, nbits;
    int     reg[3];
    sBitsreamBuffer *BS = &(state->m_MainData);
    Ipp8u  *pSrc;
    int     bitoffset;
    IppMP3FrameHeader *header = &(state->header);
#ifndef KINOMA_FAST_HUFFMAN
    IppsVLCDecodeSpec_32s *pVLCDecSpec;
    int     shift, offset, mask;
    sHuffmanTable *huff_table = state->huff_table;
#endif
    int   *non_zero = state->non_zero;
    int     part2_start = state->part2_start;

    unsigned int (*si_part23Len)[2] = state->si_part23Len;
    unsigned int (*si_bigVals)[2] = state->si_bigVals;
    unsigned int (*si_winSwitch)[2] = state->si_winSwitch;
    unsigned int (*si_blockType)[2] = state->si_blockType;
    unsigned int (*si_pTableSelect)[2][3] = state->si_pTableSelect;
    unsigned int (*si_reg0Cnt)[2] = state->si_reg0Cnt;
    unsigned int (*si_reg1Cnt)[2] = state->si_reg1Cnt;
    unsigned int (*si_cnt1TabSel)[2] = state->si_cnt1TabSel;

    sampleshort *smpl_xs = state->smpl_xs;       // out of huffman

    // the block_type is only used if the window_switching_flag is set to 1.


    if (!si_winSwitch[gr][ch] || (si_blockType[gr][ch] != 2 &&
      !(state->mpg25 == 2 && header->samplingFreq == 2))) {
        reg[0] =
            mp3dec_sfBandIndex[header->id + state->mpg25][header->samplingFreq].l[si_reg0Cnt[gr][ch] + 1];
        reg[1] =
            mp3dec_sfBandIndex[header->id + state->mpg25][header->samplingFreq].l[si_reg0Cnt[gr][ch] +
            si_reg1Cnt[gr][ch] + 2];
    } else  {
      if (si_blockType[gr][ch] == 2) {
        if (state->mpg25 == 2 && header->samplingFreq == 2)
          reg[0] = 72;
        else
          reg[0] = 36;
      } else
        reg[0] = 108;
      reg[1] = 576;
    }

    reg[2] = si_bigVals[gr][ch] * 2;

    if (reg[0] > 576)   // reg[0] upper-bounded by 576
        reg[0] = 576;
    if (reg[1] > reg[2])        // reg[1] upper-bounded by reg[2]
        reg[1] = reg[2];
    if (reg[0] > reg[2])        // reg[0] upper-bounded by reg[2]
        reg[0] = reg[2];

    pSrc = (Ipp8u *)BS->pCurrent_dword + ((32 - BS->nBit_offset) >> 3);
    bitoffset = (32 - BS->nBit_offset) & 0x7;

    i = 0;
	for (j = 0; j < 3; j++) {
        int     linbits, ii;
        short  *qp;

        idx = si_pTableSelect[gr][ch][j];
        if (idx == 4 || idx == 14) {
          return 1;
        }

#ifndef KINOMA_FAST_HUFFMAN
        pVLCDecSpec = (IppsVLCDecodeSpec_32s *) huff_table[idx].phuftable;
        linbits = huff_table[idx].linbits;
		shift = mp3dec_VLCShifts[idx];
        offset = mp3dec_VLCOffsets[idx];
        mask = (1 << (shift)) - 1;
#else
        linbits = kinoma_mp3_getlinbits(idx);
#endif	

        if (idx) {
            if (linbits == 0) {
#ifndef KINOMA_FAST_HUFFMAN
				Ipp16s  pDst[576];
                ippsVLCDecodeBlock_1u16s_x(&pSrc, &bitoffset, pDst, (reg[j] - i) >> 1,
                    pVLCDecSpec);
                qp = &(*smpl_xs)[ch][i];
                for (ii = 0; ii < (reg[j] - i) >> 1; ii++) 
				{
                    int     tmp = pDst[ii];

                    qp[0] = (short)(tmp >> shift);






                    qp[1] = (short)((tmp & mask) - offset);

                    qp += 2;
                }
#else
				kinoma_mp3_dec_huff_pair_block(&pSrc, &bitoffset,  &(*smpl_xs)[ch][i], (reg[j] - i) >> 1, idx);
#endif
            } else {

#ifndef KINOMA_FAST_HUFFMAN
				ippsVLCDecodeEscBlock_MP3_1u16s_x(&pSrc, &bitoffset, linbits,
                    &(*smpl_xs)[ch][i], (reg[j] - i),
                    pVLCDecSpec);
#else

                kinoma_mp3_dec_huff_esc_block(&pSrc, &bitoffset, &(*smpl_xs)[ch][i], (reg[j] - i), idx);
#endif
            }
            i = reg[j];

        } else {
            for (; i < reg[j]; i += 2) {
                (*smpl_xs)[ch][i] = 0;
                (*smpl_xs)[ch][i + 1] = 0;
            }
        }
    }   // for

    BS->pCurrent_dword =
        (Ipp32u *)(pSrc -
        (((Ipp32s)(pSrc) & 3) - ((Ipp32s)(BS->pCurrent_dword) & 3)));
    BS->dword = BSWAP(BS->pCurrent_dword[0]);
    BS->nBit_offset =
        32 - ((pSrc - (Ipp8u *)BS->pCurrent_dword) << 3) - bitoffset;

    idx = si_cnt1TabSel[gr][ch] + 32;

    nbits =
        si_part23Len[gr][ch] -
        (((BS->pCurrent_dword - BS->pBuffer) * 32 + 32 -
        BS->nBit_offset) - part2_start);

    if (nbits < 0) {
		//***bnie
        //vm_debug_trace(-1,
        //    VM_STRING
        //    ("Frame %d, granule %d, channel %d, error: nbits %d < 0 before zero-one part\n"),
        //    state->m_frame_num, gr, ch, nbits);
    }

#ifndef KINOMA_FAST_HUFFMAN
    pVLCDecSpec = (IppsVLCDecodeSpec_32s *) huff_table[idx].phuftable;
    shift = mp3dec_VLCShifts[idx];
    offset = mp3dec_VLCOffsets[idx];
    mask = (1 << (shift)) - 1;
#endif

    // decoding continues until all huffman bits have been decoded or until 576
    // frequency lines have been decoded
    while ((nbits > 0) && ((i + 4) <= 576)) {
        Ipp8u  *saved_pSrc;
        int     saved_bitoffset;
        int     decoded_bits;
#ifndef KINOMA_FAST_HUFFMAN
        short     tmp;
#endif

        saved_pSrc = pSrc;
        saved_bitoffset = bitoffset;

#ifndef KINOMA_FAST_HUFFMAN
        ippsVLCDecodeOne_1u16s_x(&pSrc, &bitoffset, &tmp, pVLCDecSpec);
        (*smpl_xs)[ch][i] = (short)(tmp >> (3 * shift));
        (*smpl_xs)[ch][i + 1] = (short)(((tmp >> (2 * shift)) & mask) - offset);
        (*smpl_xs)[ch][i + 2] = (short)(((tmp >> (shift)) & mask) - offset);
        (*smpl_xs)[ch][i + 3] = (short)((tmp & mask) - offset);
#else
		kinoma_mp3_dec_huff_quad( &pSrc, &bitoffset, &((*smpl_xs)[ch][i]), idx-32 );
#endif
        decoded_bits = ((pSrc - saved_pSrc) << 3) + (bitoffset - saved_bitoffset);
        nbits -= decoded_bits;
        i += 4;
    }

    if (nbits < 0) {
		//***bnie
        //vm_debug_trace(-1,
        //    VM_STRING
        //    ("Frame %d, granule %d, channel %d, error: nbits %d < 0 before zero-one part\n"),
        //    state->m_frame_num, gr, ch, nbits);

        i -= 4;
        (*smpl_xs)[ch][i] = 0;
        (*smpl_xs)[ch][i + 1] = 0;
        (*smpl_xs)[ch][i + 2] = 0;
        (*smpl_xs)[ch][i + 3] = 0;
    }

    j = i;

    for (; i < 576; i++)
        (*smpl_xs)[ch][i] = 0;

    while(j > 0 && (*smpl_xs)[ch][j - 1] == 0)
        j--;

    non_zero[ch] = j;

    BS->pCurrent_dword =
        (Ipp32u *)(pSrc -
        (((Ipp32s)(pSrc) & 3) - ((Ipp32s)(BS->pCurrent_dword) & 3)));
    BS->dword = BSWAP(BS->pCurrent_dword[0]);
    BS->nBit_offset =
        32 - ((pSrc - (Ipp8u *)BS->pCurrent_dword) << 3) - bitoffset;

    return 0;
}

/******************************************************************************
//  Name:
//    audio_data_LSF_LayerIII
//
//  Description:
//    read side information for LSF case
//
//  Input Arguments:
//    DC - point to sDecoderContext structure
//
//  Output Arguments:
//    DC - point to sDecoderContext structure, side info
//
//  Returns:
//    1 - all ok
//   -2 - wrong block type
//
******************************************************************************/
int mp3dec_audio_data_LSF_LayerIII(MP3Dec_com *state) {
    int     gr = 0, ch;
    int     tmp;

    sBitsreamBuffer *BS = &(state->m_StreamData);

    unsigned int (*si_part23Len)[2] = state->si_part23Len;
    unsigned int (*si_bigVals)[2] = state->si_bigVals;
    short   (*si_globGain)[2] = state->si_globGain;
    unsigned int (*si_sfCompress)[2] = state->si_sfCompress;
    unsigned int (*si_winSwitch)[2] = state->si_winSwitch;
    unsigned int (*si_blockType)[2] = state->si_blockType;
    unsigned int (*si_mixedBlock)[2] = state->si_mixedBlock;
    unsigned int (*si_pTableSelect)[2][3] = state->si_pTableSelect;
    short   (*si_pSubBlkGain)[2][3] = state->si_pSubBlkGain;
    unsigned int (*si_reg0Cnt)[2] = state->si_reg0Cnt;
    unsigned int (*si_reg1Cnt)[2] = state->si_reg1Cnt;
    unsigned int (*si_sfScale)[2] = state->si_sfScale;
    unsigned int (*si_cnt1TabSel)[2] = state->si_cnt1TabSel;

    int stereo = state->stereo;

    GET_BITS(BS, state->si_main_data_begin, 8);

    if (stereo == 1)
        GET_BITS(BS, state->si_private_bits, 1)
    else
        GET_BITS(BS, state->si_private_bits, 2)

    for (ch = 0; ch < stereo; ch++) {
        GET_BITS(BS, si_part23Len[gr][ch], 12);
        GET_BITS(BS, si_bigVals[gr][ch], 9);
        GET_BITS(BS, si_globGain[gr][ch], 8);
        GET_BITS(BS, si_sfCompress[gr][ch], 9);
        GET_BITS(BS, si_winSwitch[gr][ch], 1);

        if (si_winSwitch[gr][ch]) {
            GET_BITS(BS, si_blockType[gr][ch], 2);
            GET_BITS(BS, si_mixedBlock[gr][ch], 1);
            GET_BITS(BS, si_pTableSelect[gr][ch][0], 5);
            GET_BITS(BS, si_pTableSelect[gr][ch][1], 5);
            GET_BITS(BS, tmp, 3);
            si_pSubBlkGain[gr][ch][0] = (short)(-8 * tmp);
            GET_BITS(BS, tmp, 3);
            si_pSubBlkGain[gr][ch][1] = (short)(-8 * tmp);
            GET_BITS(BS, tmp, 3);
            si_pSubBlkGain[gr][ch][2] = (short)(-8 * tmp);

            if (si_blockType[gr][ch] == 0)
                return -2;    // wrong block type

            si_reg0Cnt[gr][ch] = 7 + (si_blockType[gr][ch] == 2 &&
                si_mixedBlock[gr][ch] == 0);
            si_reg1Cnt[gr][ch] = 20 - si_reg0Cnt[gr][ch];
        } else {
            GET_BITS(BS, si_pTableSelect[gr][ch][0], 5);
            GET_BITS(BS, si_pTableSelect[gr][ch][1], 5);
            GET_BITS(BS, si_pTableSelect[gr][ch][2], 5);
            GET_BITS(BS, si_reg0Cnt[gr][ch], 4);
            GET_BITS(BS, si_reg1Cnt[gr][ch], 3);

            si_blockType[gr][ch] = 0;
        }

        GET_BITS(BS, si_sfScale[gr][ch], 1);
        GET_BITS(BS, si_cnt1TabSel[gr][ch], 1);
    }

    return 1;
}

/******************************************************************************
//  Name:
//    GetScaleFactors_LSF
//
//  Description:
//    read scalefactors for LSF case
//
//  Input Arguments:
//    DC - point to sDecoderContext structure
//    ch - current channel
//
//  Output Arguments:
//    DC - point to sDecoderContext structure, side info
//
//  Returns:
//
******************************************************************************/
int mp3dec_GetScaleFactorsL3_LSF(MP3Dec_com *state, int ch) {
    int   i, j, k, sfb;
    unsigned scale_fact, int_scale_fact;
    short   scale_buffer[36];
    unsigned char *ptr;

    IppMP3FrameHeader *header = &(state->header);
    sBitsreamBuffer *BS = &(state->m_MainData);
    sScaleFactors *ScaleFactors = state->ScaleFactors;
    unsigned int (*si_blockType)[2] = state->si_blockType;
    unsigned int (*si_mixedBlock)[2] = state->si_mixedBlock;
    unsigned int (*si_sfCompress)[2] = state->si_sfCompress;
    unsigned int (*si_winSwitch)[2] = state->si_winSwitch;
    unsigned int (*si_preFlag)[2] = state->si_preFlag;

    int blocknumber = 0;
    int blocktypenumber;
    int *s_len = state->s_len;

    memset(scale_buffer, 0, sizeof(short) * 36);

    scale_fact = si_sfCompress[0][ch];
    blocktypenumber = 0;
    if ((si_blockType[0][ch] == 2) && (si_mixedBlock[0][ch] == 0))
        blocktypenumber = 1;

    if ((si_blockType[0][ch] == 2) && (si_mixedBlock[0][ch] == 1))
        blocktypenumber = 2;

    if (!(((header->modeExt == 1) || (header->modeExt == 3)) && (ch == 1))) {
        if (scale_fact < 400) {
            s_len[0] = (scale_fact >> 4) / 5;
            s_len[1] = (scale_fact >> 4) % 5;
            s_len[2] = (scale_fact % 16) >> 2;
            s_len[3] = (scale_fact % 4);
            si_preFlag[0][ch] = 0;
            blocknumber = 0;
        } else if (scale_fact < 500) {
            s_len[0] = ((scale_fact - 400) >> 2) / 5;
            s_len[1] = ((scale_fact - 400) >> 2) % 5;
            s_len[2] = (scale_fact - 400) % 4;
            s_len[3] = 0;
            si_preFlag[0][ch] = 0;
            blocknumber = 1;
        } else if (scale_fact < 512) {
            s_len[0] = (scale_fact - 500) / 3;
            s_len[1] = (scale_fact - 500) % 3;
            s_len[2] = 0;
            s_len[3] = 0;
            si_preFlag[0][ch] = 0;
            blocknumber = 2;
        }
    } else {
        int_scale_fact = scale_fact >> 1;

        if (int_scale_fact < 180) {
            s_len[0] = int_scale_fact / 36;
            s_len[1] = (int_scale_fact % 36) / 6;
            s_len[2] = (int_scale_fact % 36) % 6;
            s_len[3] = 0;
            si_preFlag[0][ch] = 0;
            blocknumber = 3;
        } else if (int_scale_fact < 244) {
            s_len[0] = ((int_scale_fact - 180) % 64) >> 4;
            s_len[1] = ((int_scale_fact - 180) % 16) >> 2;
            s_len[2] = (int_scale_fact - 180) % 4;
            s_len[3] = 0;
            si_preFlag[0][ch] = 0;
            blocknumber = 4;
        } else if (int_scale_fact < 255) {
            s_len[0] = (int_scale_fact - 244) / 3;
            s_len[1] = (int_scale_fact - 244) % 3;
            s_len[2] = 0;
            s_len[3] = 0;
            si_preFlag[0][ch] = 0;
            blocknumber = 5;
        }
    }

    k = 0;
    ptr = (unsigned char *)mp3dec_nr_of_sfb[blocknumber][blocktypenumber];

    for (i = 0; i < 4; i++) {
        int num = ptr[i];
        int len = s_len[i];
        if (len) {
            for (j = 0; j < num; j++) {
                GET_BITS(BS, scale_buffer[k++], len);
            }
        } else {
            for (j = 0; j < num; j++) {
                scale_buffer[k++] = 0;
            }
        }
    }

    k = 0;
    if (si_winSwitch[0][ch] && (si_blockType[0][ch] == 2)) {

        if (si_mixedBlock[0][ch]) {
            for (sfb = 0; sfb < 8; sfb++) {
                ScaleFactors[ch].l[sfb] = scale_buffer[k];
                k++;
            }
            for (sfb = 3; sfb < 12; sfb++)
                for (i = 0; i < 3; i++) {
                    ScaleFactors[ch].s[i][sfb] = scale_buffer[k];
                    k++;
                }

        } else {

            for (k = 0, sfb = 0; sfb < 12; sfb++)
                for (i = 0; i < 3; i++) {
                    ScaleFactors[ch].s[i][sfb] = scale_buffer[k];
                    k++;
                }
        }
    } else {
        for (sfb = 0; sfb < 21; sfb++) {
            ScaleFactors[ch].l[sfb] = scale_buffer[sfb];
        }
    }

    state->blocknumber = blocknumber;
    state->blocktypenumber = blocktypenumber;

    return 1;
}
