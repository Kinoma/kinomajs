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

#include "mp3dec_own.h"

MP3Status mp3decInit_com(MP3Dec_com *state)
{
    state->MAINDATASIZE = 2 * 56000;

    ippsZero_8u_x((Ipp8u *)&(state->header), sizeof(IppMP3FrameHeader));
    ippsZero_8u_x((Ipp8u *)&(state->header_good), sizeof(IppMP3FrameHeader));
    state->mpg25 = state->mpg25_good = 0;
    state->stereo = 0;
    state->intensity = 0;
    state->ms_stereo = 0;

    state->si_main_data_begin = 0;
    state->si_private_bits = 0;
    state->si_part23Len[0][0] = 0;
    state->si_part23Len[0][1] = 0;
    state->si_part23Len[1][0] = 0;
    state->si_part23Len[1][1] = 0;

    state->si_bigVals[0][0] = 0;
    state->si_bigVals[0][1] = 0;
    state->si_bigVals[1][0] = 0;
    state->si_bigVals[1][1] = 0;

    state->si_globGain[0][0] = 0;
    state->si_globGain[0][1] = 0;
    state->si_globGain[1][0] = 0;
    state->si_globGain[1][1] = 0;

    state->si_sfCompress[0][0] = 0;
    state->si_sfCompress[0][1] = 0;
    state->si_sfCompress[1][0] = 0;
    state->si_sfCompress[1][1] = 0;

    state->si_winSwitch[0][0] = 0;
    state->si_winSwitch[0][1] = 0;
    state->si_winSwitch[1][0] = 0;
    state->si_winSwitch[1][1] = 0;

    state->si_blockType[0][0] = 0;
    state->si_blockType[0][1] = 0;
    state->si_blockType[1][0] = 0;
    state->si_blockType[1][1] = 0;

    state->si_mixedBlock[0][0] = 0;
    state->si_mixedBlock[0][1] = 0;
    state->si_mixedBlock[1][0] = 0;
    state->si_mixedBlock[1][1] = 0;

    state->si_pTableSelect[0][0][0] = 0;
    state->si_pTableSelect[0][0][1] = 0;
    state->si_pTableSelect[0][0][2] = 0;
    state->si_pTableSelect[0][1][0] = 0;
    state->si_pTableSelect[0][1][1] = 0;
    state->si_pTableSelect[0][1][2] = 0;
    state->si_pTableSelect[1][0][0] = 0;
    state->si_pTableSelect[1][0][1] = 0;
    state->si_pTableSelect[1][0][2] = 0;
    state->si_pTableSelect[1][1][0] = 0;
    state->si_pTableSelect[1][1][1] = 0;
    state->si_pTableSelect[1][1][2] = 0;

    state->si_pSubBlkGain[0][0][0] = 0;
    state->si_pSubBlkGain[0][0][1] = 0;
    state->si_pSubBlkGain[0][0][2] = 0;
    state->si_pSubBlkGain[0][1][0] = 0;
    state->si_pSubBlkGain[0][1][1] = 0;
    state->si_pSubBlkGain[0][1][2] = 0;
    state->si_pSubBlkGain[1][0][0] = 0;
    state->si_pSubBlkGain[1][0][1] = 0;
    state->si_pSubBlkGain[1][0][2] = 0;
    state->si_pSubBlkGain[1][1][0] = 0;
    state->si_pSubBlkGain[1][1][1] = 0;
    state->si_pSubBlkGain[1][1][2] = 0;

    state->si_reg0Cnt[0][0] = 0;
    state->si_reg0Cnt[0][1] = 0;
    state->si_reg0Cnt[1][0] = 0;
    state->si_reg0Cnt[1][1] = 0;

    state->si_reg1Cnt[0][0] = 0;
    state->si_reg1Cnt[0][1] = 0;
    state->si_reg1Cnt[1][0] = 0;
    state->si_reg1Cnt[1][1] = 0;

    state->si_preFlag[0][0] = 0;
    state->si_preFlag[0][1] = 0;
    state->si_preFlag[1][0] = 0;
    state->si_preFlag[1][1] = 0;

    state->si_sfScale[0][0] = 0;
    state->si_sfScale[0][1] = 0;
    state->si_sfScale[1][0] = 0;
    state->si_sfScale[1][1] = 0;

    state->si_cnt1TabSel[0][0] = 0;
    state->si_cnt1TabSel[0][1] = 0;
    state->si_cnt1TabSel[1][0] = 0;
    state->si_cnt1TabSel[1][1] = 0;

    state->si_scfsi[0] = 0;
    state->si_scfsi[1] = 0;

    ippsZero_8u_x((Ipp8u *)&(state->ScaleFactors), 2 * sizeof(sScaleFactors));
#ifndef KINOMA_FAST_HUFFMAN
	ippsZero_8u_x((Ipp8u *)&(state->huff_table), 34 * sizeof(sHuffmanTable));
#endif
	state->part2_start = 0;

    ippsZero_8u_x((Ipp8u *)&(state->m_MainData), sizeof(sBitsreamBuffer));

    state->decodedBytes = 0;

    ippsZero_8u_x((Ipp8u *)(state->allocation), 2 * 32 * sizeof(short));
    ippsZero_8u_x((Ipp8u *)(state->scfsi), 2 * 32 * sizeof(short));
    ippsZero_8u_x((Ipp8u *)(state->scalefactor), 2 * 3 * 32 * sizeof(short));
    ippsZero_8u_x((Ipp8u *)(state->scalefactor), 2 * 32 * 36 * sizeof(unsigned short));
    state->nbal_alloc_table = NULL;
    state->alloc_table = NULL;

#ifndef KINOMA_FAST_HUFFMAN
    mp3dec_initialize_huffman(state->huff_table);
#endif

    state->m_StreamData.pBuffer = NULL;
    state->m_StreamData.pCurrent_dword = state->m_StreamData.pBuffer;
    state->m_StreamData.nDataLen = 0;
    state->m_StreamData.nBit_offset = 0;

    state->m_MainData.pBuffer = (unsigned int *)ippsMalloc_8u_x(state->MAINDATASIZE);
    if (state->m_MainData.pBuffer == NULL)
        return MP3_ALLOC;

    state->m_MainData.nBit_offset = 0;
    state->m_MainData.nDataLen = 0;
    state->m_MainData.pCurrent_dword = state->m_MainData.pBuffer;

    state->non_zero[0] = 0;
    state->non_zero[1] = 0;

    state->m_layer = 0;

    state->m_nBitrate = 0;
    state->m_frame_num = 0;
    state->m_bInit = 0;
    state->id3_size = 0;

    return MP3_OK;
}

MP3Status mp3decReset_com(MP3Dec_com *state)
{
    state->MAINDATASIZE = 2 * 56000;

    ippsZero_8u_x((Ipp8u *)&(state->header), sizeof(IppMP3FrameHeader));
//    ippsZero_8u_x((Ipp8u *)&(state->header_good), sizeof(IppMP3FrameHeader));
    state->mpg25 = 0;
//    state->mpg25_good = 0;
    state->stereo = 0;
    state->intensity = 0;
    state->ms_stereo = 0;

    state->si_main_data_begin = 0;
    state->si_private_bits = 0;
    state->si_part23Len[0][0] = 0;
    state->si_part23Len[0][1] = 0;
    state->si_part23Len[1][0] = 0;
    state->si_part23Len[1][1] = 0;

    state->si_bigVals[0][0] = 0;
    state->si_bigVals[0][1] = 0;
    state->si_bigVals[1][0] = 0;
    state->si_bigVals[1][1] = 0;

    state->si_globGain[0][0] = 0;
    state->si_globGain[0][1] = 0;
    state->si_globGain[1][0] = 0;
    state->si_globGain[1][1] = 0;

    state->si_sfCompress[0][0] = 0;
    state->si_sfCompress[0][1] = 0;
    state->si_sfCompress[1][0] = 0;
    state->si_sfCompress[1][1] = 0;

    state->si_winSwitch[0][0] = 0;
    state->si_winSwitch[0][1] = 0;
    state->si_winSwitch[1][0] = 0;
    state->si_winSwitch[1][1] = 0;

    state->si_blockType[0][0] = 0;
    state->si_blockType[0][1] = 0;
    state->si_blockType[1][0] = 0;
    state->si_blockType[1][1] = 0;

    state->si_mixedBlock[0][0] = 0;
    state->si_mixedBlock[0][1] = 0;
    state->si_mixedBlock[1][0] = 0;
    state->si_mixedBlock[1][1] = 0;

    state->si_pTableSelect[0][0][0] = 0;
    state->si_pTableSelect[0][0][1] = 0;
    state->si_pTableSelect[0][0][2] = 0;
    state->si_pTableSelect[0][1][0] = 0;
    state->si_pTableSelect[0][1][1] = 0;
    state->si_pTableSelect[0][1][2] = 0;
    state->si_pTableSelect[1][0][0] = 0;
    state->si_pTableSelect[1][0][1] = 0;
    state->si_pTableSelect[1][0][2] = 0;
    state->si_pTableSelect[1][1][0] = 0;
    state->si_pTableSelect[1][1][1] = 0;
    state->si_pTableSelect[1][1][2] = 0;

    state->si_pSubBlkGain[0][0][0] = 0;
    state->si_pSubBlkGain[0][0][1] = 0;
    state->si_pSubBlkGain[0][0][2] = 0;
    state->si_pSubBlkGain[0][1][0] = 0;
    state->si_pSubBlkGain[0][1][1] = 0;
    state->si_pSubBlkGain[0][1][2] = 0;
    state->si_pSubBlkGain[1][0][0] = 0;
    state->si_pSubBlkGain[1][0][1] = 0;
    state->si_pSubBlkGain[1][0][2] = 0;
    state->si_pSubBlkGain[1][1][0] = 0;
    state->si_pSubBlkGain[1][1][1] = 0;
    state->si_pSubBlkGain[1][1][2] = 0;

    state->si_reg0Cnt[0][0] = 0;
    state->si_reg0Cnt[0][1] = 0;
    state->si_reg0Cnt[1][0] = 0;
    state->si_reg0Cnt[1][1] = 0;

    state->si_reg1Cnt[0][0] = 0;
    state->si_reg1Cnt[0][1] = 0;
    state->si_reg1Cnt[1][0] = 0;
    state->si_reg1Cnt[1][1] = 0;

    state->si_preFlag[0][0] = 0;
    state->si_preFlag[0][1] = 0;
    state->si_preFlag[1][0] = 0;
    state->si_preFlag[1][1] = 0;

    state->si_sfScale[0][0] = 0;
    state->si_sfScale[0][1] = 0;
    state->si_sfScale[1][0] = 0;
    state->si_sfScale[1][1] = 0;

    state->si_cnt1TabSel[0][0] = 0;
    state->si_cnt1TabSel[0][1] = 0;
    state->si_cnt1TabSel[1][0] = 0;
    state->si_cnt1TabSel[1][1] = 0;

    state->si_scfsi[0] = 0;
    state->si_scfsi[1] = 0;

    ippsZero_8u_x((Ipp8u *)&(state->ScaleFactors), 2 * sizeof(sScaleFactors));
    state->part2_start = 0;

    state->decodedBytes = 0;

    ippsZero_8u_x((Ipp8u *)(state->allocation), 2 * 32 * sizeof(short));
    ippsZero_8u_x((Ipp8u *)(state->scfsi), 2 * 32 * sizeof(short));
    ippsZero_8u_x((Ipp8u *)(state->scalefactor), 2 * 3 * 32 * sizeof(short));
    ippsZero_8u_x((Ipp8u *)(state->scalefactor), 2 * 32 * 36 * sizeof(unsigned short));

    state->m_StreamData.pBuffer = NULL;
    state->m_StreamData.pCurrent_dword = state->m_StreamData.pBuffer;
    state->m_StreamData.nDataLen = 0;
    state->m_StreamData.nBit_offset = 0;

    state->m_MainData.nBit_offset = 0;
    state->m_MainData.nDataLen = 0;
    state->m_MainData.pCurrent_dword = state->m_MainData.pBuffer;

    state->non_zero[0] = 0;
    state->non_zero[1] = 0;

    state->m_layer = 0;

//    state->m_nBitrate = 0;
    state->m_frame_num = 0;
//    state->m_bInit = 0;
    state->id3_size = -1;

    return MP3_OK;
}

MP3Status mp3decClose_com(MP3Dec_com *state)
{
    int     i;

    if (state->m_MainData.pBuffer) {
        ippsFree_x(state->m_MainData.pBuffer);
        state->m_MainData.pBuffer = NULL;
    }

#ifndef KINOMA_FAST_HUFFMAN
    for (i = 1; i < 16; i++) {
        if (mp3dec_VLCBooks[i]) {
            ippsVLCDecodeFree_32s_x((IppsVLCDecodeSpec_32s *) state->huff_table[i].
                phuftable);
        }
    }

    ippsVLCDecodeFree_32s_x((IppsVLCDecodeSpec_32s *) state->huff_table[16].phuftable);
    ippsVLCDecodeFree_32s_x((IppsVLCDecodeSpec_32s *) state->huff_table[24].phuftable);
    ippsVLCDecodeFree_32s_x((IppsVLCDecodeSpec_32s *) state->huff_table[32].phuftable);
    ippsVLCDecodeFree_32s_x((IppsVLCDecodeSpec_32s *) state->huff_table[33].phuftable);
#endif

    return MP3_OK;
}
