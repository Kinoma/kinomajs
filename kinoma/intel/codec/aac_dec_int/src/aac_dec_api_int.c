/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives AAC Decode Sample for Windows*
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//  MPEG-4 and AAC are international standards promoted by ISO, IEC, ITU, ETSI
//  and other organizations. Implementations of these standards, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
*/

#ifdef KINOMA_DEBUG
extern int g_kinoma_debug_on;
#endif

/********************************************************************/
//**
#include "kinoma_ipp_lib.h"

#include <stdlib.h>
#include "aac_dec_decoding_int.h"
#include "aac_dec_own_int.h"
#include "aac_dec_huff_tables.h"
#include "sbrdec_api_int.h"
#include "sbrdec_tables_int.h"

/********************************************************************/

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

/********************************************************************/
//***
void aacdecSetIgnoreFIL(AACDec *state)
{
	state->ignoreFIL = 1;
	//state->com.ModeDecodeHEAACprofile = HEAAC_LP_MODE;
	//state->com.ModeDwnmxHEAACprofile  = HEAAC_DWNSMPL_ON;
}

AACStatus aacdecReset(AACDec *state)
{
  int i;

  if (!state)
    return AAC_NULL_PTR;

  //***
  state->ignoreFIL = 0;

  state->com.m_frame_number = 0;
  state->com.m_index_1st = 0;
  state->com.m_index_2nd = 1;
  state->com.m_index_3rd = 2;

  state->com.noiseState = 0;
  state->com.SbrFlagPresent = 0;

  if( HEAAC_PARAMS_UNDEF == state->com.ModeDecodeHEAACprofile ||
      HEAAC_PARAMS_UNDEF == state->com.ModeDwnmxHEAACprofile ){

    state->com.ModeDecodeHEAACprofile = HEAAC_HQ_MODE;
    state->com.ModeDwnmxHEAACprofile  = HEAAC_DWNSMPL_OFF;//HEAAC_DWNSMPL_ON;
  }

  for (i = 0; i < CH_MAX + COUPL_CH_MAX; i++) {
    ippsZero_16s_x(state->m_ltp_buf[i][0], 2048);
    ippsZero_16s_x(state->m_ltp_buf[i][1], 2048);
    ippsZero_16s_x(state->m_ltp_buf[i][2], 2048);
    ippsZero_32s_x(state->m_prev_samples[i], 2048);
    state->com.m_prev_win_shape[i] = 0;
  }

  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecInit(AACDec **state_ptr)
{
  AACDec *state;
  int     i;
  /* HE-AAC */
  int SizeWorkBuf = 0;

  if (!state_ptr)
    return AAC_NULL_PTR;

  state = (AACDec *)ippsMalloc_8u_x(sizeof(AACDec));

  if (state == NULL)
    return AAC_ALLOC;

  /* important: here HEAAC params must be UNDEF */

  state->com.ModeDecodeHEAACprofile = HEAAC_PARAMS_UNDEF;
  state->com.ModeDwnmxHEAACprofile  = HEAAC_PARAMS_UNDEF;

  aacdecReset(state);

  state->com.FirstID3Search = 0;
  state->com.id3_size = 0;

  state->com.m_is_chmap_valid = 0;
  state->com.m_is_pce_valid = 0;
  state->com.m_sampling_frequency = 0;
  state->com.m_sampling_frequency_index = 0;
  state->com.m_channel_number = 1;
  state->com.m_channel_number_save = 1;
  state->com.m_channel_config = 0;
  state->com.m_frame_size = 1024;
  state->com.adts_channel_configuration = -1;

  state->com.m_audio_object_type = AOT_UNDEF;

#ifndef KINOMA_FAST_HUFFMAN
  for (i = 0; i < 16; i++) {
    state->com.huffman_tables[i] = NULL;
  }
#endif

  for (i = 0; i < 10; i++) {
    state->com.sbrHuffTables[i] = NULL;
  }

  for (i = 0; i < CH_MAX; i++) {
    state->sbrBlock[i]   = NULL;
  }
  state->sbr_filter  = NULL;
  state->pWorkBuffer = NULL;


  state->m_filterbank.p_mdct_inv_long = NULL;
  state->m_filterbank.p_mdct_inv_short = NULL;
  state->m_filterbank.p_buffer_inv = NULL;
  state->m_filterbank.p_mdct_fwd_long = NULL;
  state->m_filterbank.p_mdct_fwd_short = NULL;
  state->m_filterbank.p_buffer_fwd = NULL;

#ifndef KINOMA_FAST_HUFFMAN
  for (i = 0; i < 12; i++) {
    if (ippsVLCDecodeInitAlloc_32s_x(vlcBooks[i],
                                   vlcTableSizes[i],
                                   vlcSubTablesSizes[i],
                                   vlcNumSubTables[i],
                                   (IppsVLCDecodeSpec_32s**)
                                   (&(state->com.huffman_tables[i]))) != ippStsOk) {

	  aacdecClose(state);
      return AAC_ALLOC;
    }
  }
#endif

  for (i = 0; i < 10; i++) {
    if (ippsVLCDecodeInitAlloc_32s_x(vlcSbrBooks[i],
                                   vlcSbrTableSizes[i],
                                   vlcSbrSubTablesSizes[i],
                                   vlcSbrNumSubTables[i],
                                   (IppsVLCDecodeSpec_32s**)
                                   (&(state->com.sbrHuffTables[i]))) != ippStsOk) {
      aacdecClose(state);
      return AAC_ALLOC;
    }
  }

#ifndef KINOMA_FAST_HUFFMAN
  (state->com.m_sce).stream.p_huffman_tables = (void **)&(state->com.huffman_tables[0]);
  (state->com.m_cpe).streams[0].p_huffman_tables = (void **)&(state->com.huffman_tables[0]);
  (state->com.m_cpe).streams[1].p_huffman_tables = (void **)&(state->com.huffman_tables[0]);
  (state->com.m_cce).stream.p_huffman_tables = (void **)&(state->com.huffman_tables[0]);
  (state->com.m_lfe).stream.p_huffman_tables = (void **)&(state->com.huffman_tables[0]);
#endif

  if (InitFilterbankInt(&(state->m_filterbank), FB_DECODER | FB_ENCODER) != AAC_OK) {
    aacdecClose(state);
    return AAC_ALLOC;
  }

  sbrdecInitFilter( &(state->sbr_filter), &SizeWorkBuf );
  SizeWorkBuf = UMC_MAX( SizeWorkBuf, (int)SBR_MINSIZE_OF_WORK_BUFFER );
  state->pWorkBuffer = ippsMalloc_8u_x( SizeWorkBuf );

  for (i = 0; i < CH_MAX; i++) {
    state->sbrBlock[i] = sbrInitDecoder( );
	if( state->sbrBlock[i] == NULL )
		return AAC_ALLOC;
		
    // patch
    ippsZero_32s_x(state->sbrBlock[i]->sbr_WS.AnalysisBufDelay[0],      2*320);
    ippsZero_32s_x(state->sbrBlock[i]->sbr_WS.SynthesisBufDelay[0],     2*1280);
    ippsZero_32s_x(state->sbrBlock[i]->sbr_WS.SynthesisDownBufDelay[0], 2*640);

    ippsCopy_8u_x((const Ipp8u *)(state->com.sbrHuffTables),
      (Ipp8u *)(state->sbrBlock[i]->sbr_com.sbrHuffTables),
                10 * sizeof(void *));

  }


  *state_ptr = state;
  return AAC_OK;
}

/********************************************************************/

#define RETURN_AAC_BAD_STREAM                                     \
{                                                                 \
  GET_BITS_COUNT(pBS, (*decodedBytes))                            \
  *decodedBytes = (*decodedBytes + 7) >> 3;                       \
  state_com->m_channel_number = state_com->m_channel_number_save; \
  ippsZero_16s_x(outPointer, 1024 * state_com->m_channel_number);   \
  return AAC_BAD_STREAM;                                          \
}

/********************************************************************/

AACStatus aacdecGetFrame(Ipp8u *inPointer,
                         int *decodedBytes,
                         Ipp16s *outPointer,
                         AACDec *state)
{
  sData_stream_element m_data;
  sDynamic_range_info m_drc_info;

  sBitsreamBuffer BS;
  sBitsreamBuffer *pBS = &BS;
  AACDec_com      *state_com;

  int     el_num = 0;
  int     ch_num = 0;
  int     order_counter = 0;

  int     id;
  int     tag = 0;
  int     ch = 0;
  int     ncch = 0;

/* HE-AAC param */
  int     cnt_fill_sbr_element = 0;
  int     cnt_idaac_sbr_element = 0;
  int     sbrFlagPresentLFE = 0;
  int     NumRealCh = 0;
  int     ch_counter;
  int     scaleFactorCoreAAC;
  int     scaleFactorAAC;
  int     errSBRParser = 0;

  s_tns_data tns_data0;
  int     i, j, m_channel_number;

  if (!inPointer || !outPointer)
    return AAC_NULL_PTR;

  GET_INIT_BITSTREAM(pBS, inPointer)

  // init
  state_com = &(state->com);
  state->com.m_up_sample  = 1;

  GET_BITS(pBS, id, 3)
#ifdef KINOMA_DEBUG
	if( g_kinoma_debug_on)
		fprintf( stderr, "###kinoma debug: id:%d\n", id);
#endif
  while (id != ID_END) {
    if (CH_MAX - 1 < NumRealCh)
      RETURN_AAC_BAD_STREAM;
    switch (id) {
    case ID_SCE:
      /* may be there is HE-AAC element */
      state->sbrBlock[cnt_idaac_sbr_element++]->sbr_com.id_aac = ID_SCE;
      NumRealCh++;

      /* Parsing bitstream */
      if (dec_sce_channel_element(&(state_com->m_sce), pBS, state_com->m_audio_object_type) < 0)
        RETURN_AAC_BAD_STREAM;
      tag = (state_com->m_sce).element_instance_tag;
      break;
    case ID_CPE:
      /* may be there is HE-AAC element */
      state->sbrBlock[cnt_idaac_sbr_element++]->sbr_com.id_aac = ID_CPE;
      NumRealCh += 2;

      /* Parsing bitstream */
      if (dec_cpe_channel_element(&(state_com->m_cpe), pBS, state_com->m_audio_object_type) < 0)
        RETURN_AAC_BAD_STREAM;
      tag = (state_com->m_cpe).element_instance_tag;
      break;
    case ID_CCE:
      if (COUPL_CH_MAX - 1 < ncch)
        RETURN_AAC_BAD_STREAM;
      if (dec_coupling_channel_element(&(state_com->m_cce), &(state_com->m_cdata[ncch]),
            pBS, state_com->m_audio_object_type) < 0)
        RETURN_AAC_BAD_STREAM;
      tag = (state_com->m_cce).element_instance_tag;
      break;
    case ID_LFE:
      /* may be there is HE-AAC element */
      if (CH_MAX - 1 < cnt_idaac_sbr_element)
        RETURN_AAC_BAD_STREAM;

      state->sbrBlock[cnt_idaac_sbr_element++]->sbr_com.id_aac = ID_LFE;

      if (cnt_fill_sbr_element > 0) {
        sbrFlagPresentLFE++;

        if ( CH_MAX - 1 < cnt_fill_sbr_element )
          RETURN_AAC_BAD_STREAM;

        cnt_fill_sbr_element++;
      }


      /* Parsing bitstream */
      if (dec_lfe_channel_element(&(state_com->m_lfe), pBS, state_com->m_audio_object_type) < 0)
        RETURN_AAC_BAD_STREAM;
      tag = (state_com->m_lfe).element_instance_tag;
      NumRealCh++;
      break;
    case ID_DSE:
      dec_data_stream_element(&m_data, pBS);
      break;
    case ID_PCE:
      dec_program_config_element(&(state_com->m_pce), pBS);
      state_com->m_is_chmap_valid = 0;
      state_com->m_is_pce_valid = 1;
      break;
	case ID_FIL:
		if( state->ignoreFIL )		//***to turn off sbr  --bnie  1/22/07
			break;
#ifdef KINOMA_DEBUG
	if( g_kinoma_debug_on)
		fprintf( stderr, "   kinoma debug: FIL not ignored\n");
#endif

/*
* NOTES:
* (1)SBR FILL ELEMENT there is for SCE & CPE modes only.
* (2)If AOT AAC is MAIN, LC or LTP
* then
*   <SCE> <FIL <EXT_SBR_DATA(SCE)>> // center
*   <CPE> <FIL <EXT_SBR_DATA(CPE)>> // front L/R
*   <CPE> <FIL <EXT_SBR_DATA(CPE)>> // back L/R
*   <LFE> // sub
*   <END> // (end of raw data block)
*
* (3)If AOT AAC is ER LC or ER LTP
* then
*   <SCE> <CPE> <CPE> <LFE> <EXT <SBR(SCE)> <SBR(CPE)> <SBR(CPE)>>
*
* (4) may be more then one FILL ELEMENT, for ex:
*  <SCE> <FILL.. <CPE> <FILL.. <CPE> <FILL..><FILL..> <LFE><FILL..><FILL..><FILL..>
*/
      if ( CH_MAX - 1 < cnt_fill_sbr_element )
        RETURN_AAC_BAD_STREAM;

      state->sbrBlock[cnt_fill_sbr_element]->sbr_com.FreqSbr =
        state_com->m_sampling_frequency * 2;

      if (state->com.m_sampling_frequency_index >= 3)
        state->sbrBlock[cnt_fill_sbr_element]->sbr_com.sbr_freq_indx =
          state->com.m_sampling_frequency_index - 3;

      state->sbrBlock[cnt_fill_sbr_element]->sbr_com.sbrFlagError = 0;

      errSBRParser = dec_fill_element(&(state->sbrBlock[cnt_fill_sbr_element]->sbr_com),
                                       &cnt_fill_sbr_element, &m_drc_info, pBS);

      /* patch while fixed point sbr isn't implemented
      {
        sSbrDecCommon SbrBlock;
        dec_fill_element(&SbrBlock, &cnt_fill_sbr_element, &m_drc_info, pBS);
      }
      */

      /*************************************************************
       * if there is problem with HE-AAC parser then QUIT
       *************************************************************/
      if ( errSBRParser )
        RETURN_AAC_BAD_STREAM

      break;
    }

    if ((ID_DSE > id) && (ID_CCE != id)) {
      if (0 == state_com->m_is_chmap_valid) {
        state_com->m_chmap[id][tag].ch = (short)ch_num;
        state_com->m_elmap[el_num].id = (short)id;
        state_com->m_elmap[el_num].tag = (short)tag;
        state_com->m_elmap[el_num].ch = (short)ch_num;
        state_com->m_order[order_counter++] = ch_num;

        el_num++;
        ch_num++;
        if (ID_CPE == id) {
          state_com->m_order[order_counter++] = ch_num;
          ch_num++;
        }
      }
      ch = state_com->m_chmap[id][tag].ch;
    }

    switch (id) {
    case ID_SCE:

      state_com->m_curr_win_shape[ch] = (state_com->m_sce).stream.window_shape;
      state_com->m_curr_win_sequence[ch] = (state_com->m_sce).stream.window_sequence;

      if (0 != (state_com->m_sce).stream.pulse_data_present) {
        ics_apply_pulse_I(&(state_com->m_sce).stream);
      }

      ics_apply_scale_factors(&(state_com->m_sce).stream, state->m_spectrum_data[ch]);

      apply_pns(&(state_com->m_sce).stream, NULL, state->m_spectrum_data[ch], NULL, 1,
                0, NULL, &(state_com->noiseState));

      deinterlieve(&(state_com->m_sce).stream, state->m_spectrum_data[ch]);
      ics_calc_tns_data(&(state_com->m_sce).stream, &(state->tns_data[ch]));

      if (AOT_AAC_LTP == state_com->m_audio_object_type) {
        state->m_ltp.p_samples_1st_part = state->m_ltp_buf[ch][state_com->m_index_1st];
        state->m_ltp.p_samples_2nd_part = state->m_ltp_buf[ch][state_com->m_index_2nd];
        state->m_ltp.p_samples_3rd_part = state->m_ltp_buf[ch][state_com->m_index_3rd];
        state->m_ltp.prev_windows_shape = state_com->m_prev_win_shape[ch];

        state->m_ltp.p_filterbank_data = &(state->m_filterbank);
        state->m_ltp.p_tns_data = &(state->tns_data[ch]);

        ics_apply_ltp_I(&(state->m_ltp), &(state_com->m_sce).stream, state->m_spectrum_data[ch]);
      }
      break;
    case ID_CPE:

      state_com->m_curr_win_shape[ch] = (state_com->m_cpe).streams[0].window_shape;
      state_com->m_curr_win_sequence[ch] = (state_com->m_cpe).streams[0].window_sequence;
      state_com->m_curr_win_shape[ch + 1] = (state_com->m_cpe).streams[1].window_shape;
      state_com->m_curr_win_sequence[ch + 1] = (state_com->m_cpe).streams[1].window_sequence;

      if (0 != (state_com->m_cpe).streams[0].pulse_data_present) {
        ics_apply_pulse_I(&(state_com->m_cpe).streams[0]);
      }
      if (0 != (state_com->m_cpe).streams[1].pulse_data_present) {
        ics_apply_pulse_I(&(state_com->m_cpe).streams[1]);
      }

      ics_apply_scale_factors(&(state_com->m_cpe).streams[0], state->m_spectrum_data[ch]);
      ics_apply_scale_factors(&(state_com->m_cpe).streams[1],
                              state->m_spectrum_data[ch + 1]);

      /* Joint stereo */
      cpe_apply_ms(&(state_com->m_cpe), state->m_spectrum_data[ch],
                    state->m_spectrum_data[ch + 1]);

      apply_pns(&(state_com->m_cpe).streams[0], &(state_com->m_cpe).streams[1],
                state->m_spectrum_data[ch], state->m_spectrum_data[ch + 1], 2,
                (state_com->m_cpe).ms_mask_present, (state_com->m_cpe).ms_used,
                &(state_com->noiseState));

      cpe_apply_intensity(&(state_com->m_cpe), state->m_spectrum_data[ch],
                          state->m_spectrum_data[ch + 1]);


      ics_calc_tns_data(&(state_com->m_cpe).streams[0], &(state->tns_data[ch]));
      ics_calc_tns_data(&(state_com->m_cpe).streams[1], &(state->tns_data[ch + 1]));

      deinterlieve(&(state_com->m_cpe).streams[0], state->m_spectrum_data[ch]);
      deinterlieve(&(state_com->m_cpe).streams[1], state->m_spectrum_data[ch + 1]);

      if (AOT_AAC_LTP == state_com->m_audio_object_type) {
        state->m_ltp.p_samples_1st_part = state->m_ltp_buf[ch][state_com->m_index_1st];
        state->m_ltp.p_samples_2nd_part = state->m_ltp_buf[ch][state_com->m_index_2nd];
        state->m_ltp.p_samples_3rd_part = state->m_ltp_buf[ch][state_com->m_index_3rd];
        state->m_ltp.prev_windows_shape = state_com->m_prev_win_shape[ch];

        state->m_ltp.p_filterbank_data = &(state->m_filterbank);
        state->m_ltp.p_tns_data = &(state->tns_data[ch]);

        ics_apply_ltp_I(&(state->m_ltp), &(state_com->m_cpe).streams[0],
                        state->m_spectrum_data[ch]);

        state->m_ltp.p_samples_1st_part = state->m_ltp_buf[ch + 1][state_com->m_index_1st];
        state->m_ltp.p_samples_2nd_part = state->m_ltp_buf[ch + 1][state_com->m_index_2nd];
        state->m_ltp.p_samples_3rd_part = state->m_ltp_buf[ch + 1][state_com->m_index_3rd];
        state->m_ltp.prev_windows_shape = state_com->m_prev_win_shape[ch + 1];

        state->m_ltp.p_tns_data = &(state->tns_data[ch + 1]);

        ics_apply_ltp_I(&(state->m_ltp), &(state_com->m_cpe).streams[1],
                        state->m_spectrum_data[ch + 1]);
      }
      break;
    case ID_CCE:

      state_com->m_curr_win_shape[CH_MAX + ncch] =
        (state_com->m_cce).stream.window_shape;
      state_com->m_curr_win_sequence[CH_MAX + ncch] =
        (state_com->m_cce).stream.window_sequence;

      if (0 != (state_com->m_cce).stream.pulse_data_present) {
        ics_apply_pulse_I(&(state_com->m_cce).stream);
      }

      ics_apply_scale_factors(&(state_com->m_cce).stream,
        state->m_spectrum_data[CH_MAX + ncch]);

      apply_pns(&(state_com->m_cce).stream, NULL,
                state->m_spectrum_data[CH_MAX + ncch], NULL, 1,
                0, NULL, &(state_com->noiseState));

      deinterlieve(&(state_com->m_cce).stream,
        state->m_spectrum_data[CH_MAX + ncch]);

      memset(&tns_data0, 0, sizeof(tns_data0));
      ics_calc_tns_data(&(state_com->m_cce).stream, &tns_data0);

      if (AOT_AAC_LTP == state_com->m_audio_object_type) {
        state->m_ltp.p_samples_1st_part =
          state->m_ltp_buf[CH_MAX + ncch][state_com->m_index_1st];
        state->m_ltp.p_samples_2nd_part =
          state->m_ltp_buf[CH_MAX + ncch][state_com->m_index_2nd];
        state->m_ltp.p_samples_3rd_part =
          state->m_ltp_buf[CH_MAX + ncch][state_com->m_index_3rd];
        state->m_ltp.prev_windows_shape =
          state_com->m_prev_win_shape[CH_MAX + ncch];

        state->m_ltp.p_filterbank_data = &(state->m_filterbank);
        state->m_ltp.p_tns_data = &tns_data0;

        ics_apply_ltp_I(&(state->m_ltp), &(state_com->m_cce).stream,
          state->m_spectrum_data[CH_MAX + ncch]);
      }

      if (0 != tns_data0.m_tns_data_present) {
        ics_apply_tns_dec_I(&tns_data0, state->m_spectrum_data[CH_MAX + ncch]);
      }

      coupling_gain_calculation(&(state_com->m_cce), &(state_com->m_cdata[ncch]),
                                state->cc_gain[ncch], state->cc_gain_factor[ncch]);
      ncch++;
      break;
    case ID_LFE:
      state_com->m_curr_win_shape[ch] = (state_com->m_lfe).stream.window_shape;
      state_com->m_curr_win_sequence[ch] = (state_com->m_lfe).stream.window_sequence;

      if (0 != (state_com->m_lfe).stream.pulse_data_present) {
        ics_apply_pulse_I(&(state_com->m_lfe).stream);
      }
      ics_apply_scale_factors(&(state_com->m_lfe).stream, state->m_spectrum_data[ch]);
      state->tns_data[ch].m_tns_data_present = 0;
      break;
    default:
      break;
    }
    GET_BITS(pBS, id, 3)

#ifdef KINOMA_DEBUG
	if( g_kinoma_debug_on)
		fprintf( stderr, "   kinoma debug: id:%d\n", id);
#endif
  }

  if (0 == state_com->m_is_chmap_valid) {

    if (el_num <= 0)
      RETURN_AAC_BAD_STREAM

    state_com->m_is_chmap_valid = 1;

    if (1 == state_com->m_is_pce_valid) {
      chmap_create_by_pce(&(state_com->m_pce), state_com->m_chmap);
      ch_num = chmap_order(state_com->m_chmap, state_com->m_elmap,
                           el_num, state_com->m_order);
    } else if (state_com->adts_channel_configuration >= 0) {
      chmap_create_by_adts(state_com->adts_channel_configuration,
                           state_com->m_chmap, state_com->m_elmap, el_num,
                           state_com->m_order);
      ch_num = chmap_order(state_com->m_chmap, state_com->m_elmap,
                           el_num, state_com->m_order);
    }
    for (i = 0; i < ch_num; i++) {
      ch = state_com->m_order[i];
      state->m_ordered_samples[i] = state->m_curr_samples[ch];
    }

    if (ch_num <= 0)
      RETURN_AAC_BAD_STREAM

    state_com->m_channel_number = ch_num;
    state_com->m_element_number = el_num;
  }

  if (AOT_AAC_LTP == state_com->m_audio_object_type) {
    state_com->m_index_1st++;
    state_com->m_index_1st %= 3;
    state_com->m_index_2nd++;
    state_com->m_index_2nd %= 3;
    state_com->m_index_3rd++;
    state_com-> m_index_3rd %= 3;
  }

  /* coupling channel process */
  for (ch = 0; ch < ncch; ch++) {
    int c;
    for (c = 0; c < state_com->m_cdata[ch].num_coupled_elements + 1; c++) {
      int id = state_com->m_cdata[ch].cc_target_id[c];
      int tag = state_com->m_cdata[ch].cc_target_tag[c];
      state_com->m_cdata[ch].cc_target_ch[c] = -1;
      for (i = 0; i < state_com->m_element_number; i++) {
        if ((state_com->m_elmap[i].id == id) &&
            (state_com->m_elmap[i].tag == tag)) {
          state_com->m_cdata[ch].cc_target_ch[c] = state_com->m_elmap[i].ch;
          break;
        }
      }
    }
  }

  for (ch = 0; ch < ncch; ch++) {
    if ((state_com->m_cdata[ch].ind_sw_cce_flag) ||
        (AOT_AAC_LTP == state_com->m_audio_object_type)) {

      FilterbankDecInt(&(state->m_filterbank),
                       state->m_spectrum_data[CH_MAX + ch],
                       state->m_prev_samples[CH_MAX + ch],
                       state_com->m_curr_win_sequence[CH_MAX + ch],
                       state_com->m_curr_win_shape[CH_MAX + ch],
                       state_com->m_prev_win_shape[CH_MAX + ch],
                       state->m_curr_samples[CH_MAX + ch],
                       state->m_prev_samples[CH_MAX + ch]);

      state_com->m_prev_win_shape[CH_MAX + ch] =
        state_com->m_curr_win_shape[CH_MAX + ch];

      /* Update buffers for LTP */
      if (AOT_AAC_LTP == state_com->m_audio_object_type) {
        ippsConvert_32s16s_Sfs_x(state->m_curr_samples[CH_MAX + ch],
                             state->m_ltp_buf[ch][state_com->m_index_2nd],
                             1024, 8);

        ippsConvert_32s16s_Sfs_x(state->m_prev_samples[CH_MAX + ch],
                               state->m_ltp_buf[ch][state_com->m_index_3rd],
                               1024, 8);
      }
    }
  }

  for (ch = 0; ch < ncch; ch++) {
    if ((!state_com->m_cdata[ch].ind_sw_cce_flag) &&
        (!state_com->m_cdata[ch].cc_domain)) {
    coupling_spectrum(state, &(state_com->m_cdata[ch]),
                      state->m_spectrum_data[CH_MAX + ch],
                      state_com->m_curr_win_sequence[CH_MAX + ch],
                      state->cc_gain[ch], state->cc_gain_factor[ch]);
    }
  }

  for (i = 0; i < state_com->m_channel_number; i++) {
    ch = state_com->m_order[i];
    if (0 != state->tns_data[ch].m_tns_data_present) {
      ics_apply_tns_dec_I(&(state->tns_data[ch]), state->m_spectrum_data[ch]);
    }
  }

  for (ch = 0; ch < ncch; ch++) {
    if ((!state_com->m_cdata[ch].ind_sw_cce_flag) &&
        (state_com->m_cdata[ch].cc_domain)) {
    coupling_spectrum(state, &(state_com->m_cdata[ch]),
                      state->m_spectrum_data[CH_MAX + ch],
                      state_com->m_curr_win_sequence[CH_MAX + ch],
                      state->cc_gain[ch], state->cc_gain_factor[ch]);
    }
  }

  for (ch_counter = 0; ch_counter < state_com->m_channel_number; ch_counter++) {
    ch = state_com->m_order[ch_counter];

    FilterbankDecInt(&(state->m_filterbank), state->m_spectrum_data[ch],
                  state->m_prev_samples[ch], state_com->m_curr_win_sequence[ch],
                  state_com->m_curr_win_shape[ch], state_com->m_prev_win_shape[ch],
                  state->m_curr_samples[ch], state->m_prev_samples[ch]);

    state_com->m_prev_win_shape[ch] = state_com->m_curr_win_shape[ch];
  }

  for (ch = 0; ch < ncch; ch++) {
    if (state_com->m_cdata[ch].ind_sw_cce_flag) {
    coupling_samples(state, &(state_com->m_cdata[ch]),
                     state->m_curr_samples[CH_MAX + ch],
                     state->cc_gain[ch], state->cc_gain_factor[ch]);
    }
  }

  for (ch_counter = 0; ch_counter < state_com->m_channel_number; ch_counter++) {
    ch = state_com->m_order[ch_counter];

    /* Update buffers for LTP */
    if (AOT_AAC_LTP == state_com->m_audio_object_type) {
      ippsConvert_32s16s_Sfs_x(state->m_curr_samples[ch],
                             state->m_ltp_buf[ch][state_com->m_index_2nd],
                             1024, 8);

      ippsConvert_32s16s_Sfs_x(state->m_prev_samples[ch],
                             state->m_ltp_buf[ch][state_com->m_index_3rd],
                             1024, 8);
    }
  }

    /* HE-AAC patch */
  if ((sbrFlagPresentLFE) && (cnt_fill_sbr_element)) {

    /* we passed LFE if ER AAC mode is used */
    if (cnt_fill_sbr_element != cnt_idaac_sbr_element) {
      cnt_fill_sbr_element++;
    }

    /* may be error? if yes then we isn't doing sbr process */
    if (cnt_fill_sbr_element != cnt_idaac_sbr_element) {
      cnt_fill_sbr_element = 0;
    }

  }

  // be careful !!!
  scaleFactorCoreAAC = 8;
  scaleFactorAAC = scaleFactorCoreAAC;

  for (i = 0, j = 0; i < cnt_fill_sbr_element; i++) {

    scaleFactorAAC = scaleFactorCoreAAC;


    sbrGetFrame(state->m_curr_samples[j],// implace call
                state->m_curr_samples[j],
                state->sbrBlock[i], state->sbr_filter,
                0, //ch L
                state_com->ModeDecodeHEAACprofile,
                state_com->ModeDwnmxHEAACprofile,
                state->pWorkBuffer,
                &scaleFactorAAC ); //sf will be changed
#ifdef KINOMA_DEBUG
	if( g_kinoma_debug_on)
	{
		fprintf( stderr, "   kinoma debug: sbrGetFrame, scaleFactorAAC:%d\n", scaleFactorAAC);

		{
			int *p = state->m_ordered_samples[0];
			fprintf( stderr, "   kinoma debug: out:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p[0],p[1],p[2],p[3],p[4],p[5] );
		}
	}
#endif
    j++;

    if (state->sbrBlock[i]->sbr_com.id_aac == ID_CPE) {


  /* we "forget" New scaleFactorAAC from L_CHANNEL
   * because OUT scaleFactor for L & R channel the same
   */

      scaleFactorAAC = scaleFactorCoreAAC;

      sbrGetFrame(state->m_curr_samples[j],// implace call
                  state->m_curr_samples[j],
                  state->sbrBlock[i],
                  state->sbr_filter,
                  1,//ch R
                  state_com->ModeDecodeHEAACprofile,
                  state_com->ModeDwnmxHEAACprofile,
                  state->pWorkBuffer,
                  &scaleFactorAAC );//sf will be changed

      j++;
    }
  }

  if (cnt_fill_sbr_element) {
    if (state_com->ModeDwnmxHEAACprofile == HEAAC_DWNSMPL_OFF) {
      state->com.m_up_sample  = 2;
    }
    state_com->SbrFlagPresent = 1;
  }

  m_channel_number = state_com->m_channel_number;

  if (m_channel_number == 1) {
    for (j = 0; j < m_channel_number; j++) {
      Ipp16s *tmpPtr = outPointer + j;
      int num = state->com.m_up_sample * 1024;
      int *srcPrt = state->m_ordered_samples[j];

#ifdef KINOMA_DEBUG
	if( g_kinoma_debug_on)
		fprintf( stderr, "   kinoma debug: num:%d\n", num);
#endif

      for (i = 0; i < num; i++) {
        int tmp = *srcPrt;
        int tmp0;
        tmp = (tmp + ( 1 << (scaleFactorAAC -1) ) ) >> scaleFactorAAC;
        tmp0 = tmp;
        if (tmp > 32767) tmp0 = 32767;
        if (tmp < -32768) tmp0 = -32768;
        *tmpPtr = (Ipp16s)tmp0;
        tmpPtr++;
        srcPrt++;
      }
    }
  } else if (m_channel_number == 2) {
    for (j = 0; j < m_channel_number; j++) {
      Ipp16s *tmpPtr = outPointer + j;
      int num = state->com.m_up_sample * 1024;
      int *srcPrt = state->m_ordered_samples[j];

      for (i = 0; i < num; i++) {
        int tmp = *srcPrt;
        int tmp0;
        tmp = (tmp + ( 1 << (scaleFactorAAC -1) ) ) >> scaleFactorAAC;
        tmp0 = tmp;
        if (tmp > 32767) tmp0 = 32767;
        if (tmp < -32768) tmp0 = -32768;
        *tmpPtr = (Ipp16s)tmp0;
        tmpPtr += 2;
        srcPrt++;
      }
    }
  } else {
    for (j = 0; j < m_channel_number; j++) {
      Ipp16s *tmpPtr = outPointer + j;
      int num = state->com.m_up_sample * 1024;
      int *srcPrt = state->m_ordered_samples[j];

      for (i = 0; i < num; i++) {
        int tmp = *srcPrt;
        int tmp0;
        tmp = (tmp + ( 1 << (scaleFactorAAC -1) ) ) >> scaleFactorAAC;
        tmp0 = tmp;
        if (tmp > 32767) tmp0 = 32767;
        if (tmp < -32768) tmp0 = -32768;
        *tmpPtr = (Ipp16s)tmp0;
        tmpPtr += m_channel_number;
        srcPrt++;
      }
    }
  }

  Byte_alignment(pBS);
  GET_BITS_COUNT(pBS, (*decodedBytes))
  *decodedBytes >>= 3;
  state_com->m_frame_number++;
  state_com->m_channel_number_save = state_com->m_channel_number;

  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecSetSamplingFrequency(int sampling_frequency_index,
                                     AACDec *state)
{
  return aacdecSetSamplingFrequencyCom(sampling_frequency_index,
                                       &(state->com));
}

/********************************************************************/

AACStatus aacdecSetSBRModeDecode(int ModeDecodeHEAAC,
                                 AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  state->com.ModeDecodeHEAACprofile = ModeDecodeHEAAC;

  return AAC_OK;
}


/********************************************************************/

AACStatus aacdecSetSBRModeDwnmx(int ModeDwnsmplHEAAC,
                                AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  state->com.ModeDwnmxHEAACprofile = ModeDwnsmplHEAAC;

  return AAC_OK;
}


/********************************************************************/

AACStatus aacdecSetAudioObjectType(enum AudioObjectType audio_object_type,
                                   AACDec *state)
{

  if (!state)
    return AAC_NULL_PTR;

  state->com.m_audio_object_type = audio_object_type;
  return AAC_OK;
}


/********************************************************************/

AACStatus aacdecSetPCE(sProgram_config_element *pce,
                       AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  ippsCopy_8u_x((Ipp8u*)pce, (Ipp8u*)(&(state->com.m_pce)),
              sizeof(sProgram_config_element));

  state->com.m_is_pce_valid = 1;

  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecSetAdtsChannelConfiguration(int adts_channel_configuration,
                                            AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  if ((adts_channel_configuration < 0) || (adts_channel_configuration > 7))
    return AAC_BAD_PARAMETER;

  state->com.adts_channel_configuration = adts_channel_configuration;
  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecGetSbrFlagPresent(int *SbrFlagPresent,
                                  AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  *SbrFlagPresent = state->com.SbrFlagPresent;
  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecGetFrameSize(int *frameSize,
                             AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  *frameSize = state->com.m_frame_size * state->com.m_up_sample;
  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecGetSampleFrequency(int *freq,
                                   AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  *freq = state->com.m_sampling_frequency * state->com.m_up_sample;
  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecGetChannels(int *ch,
                            AACDec *state)
{
  if (!state)
    return AAC_NULL_PTR;

  *ch = state->com.m_channel_number;
  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecClose(AACDec *state)
{
  int     i;

  if (state == NULL)
    return AAC_OK;

  FreeFilterbankInt(&(state->m_filterbank));

#ifndef KINOMA_FAST_HUFFMAN
  for (i = 0; i < 16; i++) {
    if (state->com.huffman_tables[i]) {
      ippsVLCDecodeFree_32s_x((IppsVLCDecodeSpec_32s *) (state->com.huffman_tables[i]));
    }
  }
#endif

  sbrdecFreeFilter( state->sbr_filter );

  for (i = 0; i < CH_MAX; i++)
    sbrFreeDecoder(state->sbrBlock[i]);

  for (i = 0; i < 10; i++)
    ippsVLCDecodeFree_32s_x((IppsVLCDecodeSpec_32s *) (state->com.sbrHuffTables[i]));

  if( state->pWorkBuffer )
    ippsFree_x( state->pWorkBuffer );

  ippsFree_x(state);

  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecGetInfo(cAudioCodecParams *a_info,
                        AACDec *state)
{
  if (!a_info)
    return AAC_NULL_PTR;

  a_info->m_SuggestedInputSize = 6 * 1024;

  if (state->com.m_frame_number > 0) {
    a_info->m_info_in.bitPerSample = 0;
    a_info->m_info_out.bitPerSample = 16;

    a_info->m_info_in.bitrate = 0;
    a_info->m_info_out.bitrate = 0;

    a_info->m_info_in.channels = state->com.m_channel_number;
    a_info->m_info_out.channels = state->com.m_channel_number;

    a_info->m_info_in.sample_frequency = state->com.m_sampling_frequency;
    a_info->m_info_out.sample_frequency = state->com.m_sampling_frequency * state->com.m_up_sample;

    a_info->m_info_in.stream_type = AAC_AUD;
    a_info->m_info_out.stream_type = PCM_AUD;

    a_info->m_frame_num = state->com.m_frame_number;

    a_info->is_valid = 1;

    return AAC_OK;
  }

  a_info->is_valid = 0;
  return AAC_OK;
}

/********************************************************************/

AACStatus aacdecGetDuration(float *p_duration,
                            AACDec *state)
{
  float   duration;

  duration = (float)(state->com.m_frame_number) * 1024;
  *p_duration = duration / (float)(state->com.m_sampling_frequency);

  return AAC_OK;
}

/********************************************************************/

AACStatus aacdec_GetID3Len(Ipp8u *in,
                           int inDataSize,
                           AACDec *state)
{
  if (state->com.FirstID3Search != 0)
    return AAC_OK;

  if (inDataSize < 10)
    return AAC_NOT_ENOUGH_DATA;

  if ((in[0] == 'I') && (in[1] == 'D') && (in[2] == '3') && /* 'ID3' */
      (in[3] < 0xFF) && (in[4] < 0xFF) &&                   /* Version or revision will never be 0xFF */
      (in[6] < 0x80) && (in[7] < 0x80) && (in[8] < 0x80) && (in[9] < 0x80)) { /* size */
    state->com.id3_size = (in[6] << 21) + (in[7] << 14) + (in[8] << 7) + in[9] +
      (in[5] & 0x10 ? 20 : 10);
  } else {
    state->com.id3_size = 0;
  }

  state->com.FirstID3Search = 1;

  return AAC_OK;
}

/********************************************************************/

AACStatus aacdec_SkipID3(int inDataSize,
                         int *skipped,
                         AACDec *state)
{
  *skipped = 0;
  if (state->com.id3_size > 0) {
    if (inDataSize < state->com.id3_size) {
      *skipped = inDataSize;
      state->com.id3_size -= inDataSize;
      return AAC_NOT_ENOUGH_DATA;
    } else {
      *skipped = state->com.id3_size;
      state->com.id3_size = 0;
      return AAC_OK;
    }
  }
  return AAC_OK;
}

/********************************************************************/
