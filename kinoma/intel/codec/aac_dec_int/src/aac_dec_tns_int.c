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

/********************************************************************/

#include "aac_dec_tns_int.h"

/********************************************************************/

static void tns_ar_filter_plus(Ipp32s *p_spectrum,
                               int size,
                               Ipp32s *lpc_coef,
                               int    scalef,
                               int order);
static void tns_ar_filter_minus(Ipp32s *p_spectrum,
                                int size,
                                Ipp32s *lpc_coef,
                                int    scalef,
                                int order);
static void tns_ma_filter(Ipp32s *p_spectrum,
                          int size,
                          int inc,
                          Ipp32s *lpc_coef,
                          int    scalef,
                          int order);

/********************************************************************/

void ics_apply_tns_enc_I(s_tns_data *p_data,
                         Ipp32s *p_spectrum)
{
  int     w;
  int     f;

  for (w = 0; w < p_data->m_num_windows; w++) {
    for (f = 0; f < p_data->m_n_filt[w]; f++) {
      if (0 == p_data->m_order[w][f])
        continue;
      tns_ma_filter(&(p_spectrum[w * 128 + p_data->m_start[w][f]]),
                    p_data->m_size[w][f], p_data->m_inc[w][f],
                    p_data->m_lpc[w][f], p_data->m_lpc_scalef[w][f],
                    p_data->m_order[w][f]);
    }
  }
}

/********************************************************************/

void ics_apply_tns_dec_I(s_tns_data *p_data,
                         Ipp32s *p_spectrum)
{
  int     w;
  int     f;

  for (w = 0; w < p_data->m_num_windows; w++) {
    for (f = 0; f < p_data->m_n_filt[w]; f++) {
      if (0 == p_data->m_order[w][f])
        continue;

      if (p_data->m_inc[w][f] > 0) {
        tns_ar_filter_plus(&(p_spectrum[w * 128 + p_data->m_start[w][f]]),
                           p_data->m_size[w][f], p_data->m_lpc[w][f],
                           p_data->m_lpc_scalef[w][f], p_data->m_order[w][f]);
      } else {
        tns_ar_filter_minus(&(p_spectrum[w * 128 + p_data->m_start[w][f]]),
                            p_data->m_size[w][f], p_data->m_lpc[w][f],
                            p_data->m_lpc_scalef[w][f], p_data->m_order[w][f]);
      }
    }
  }
}

/********************************************************************/

static void tns_decode_coef(int order,
                            int coef_res_bits,
                            int coef_compress,
                            int *coef,
                            Ipp32s *a,
                            Ipp32s *scale);

/********************************************************************/

void ics_calc_tns_data(s_SE_Individual_channel_stream *p_stream,
                       s_tns_data *p_data)
{
  int     w;
  int     f;
  int     bottom;
  int     top;
  int     tns_order;
  int     min_sfb;
  int     tns_max_bands;
  int     tns_max_order;
  int     start;
  int     end;
  int    *p_sfb_offset;
  int     size;
  int     num_swb;

  p_data->m_tns_data_present = p_stream->tns_data_present;

  if (0 == p_stream->tns_data_present)
    return;

  if (EIGHT_SHORT_SEQUENCE == p_stream->window_sequence) {
    tns_max_bands = p_stream->tns_max_bands_short;
    tns_max_order = p_stream->tns_max_order_short;
    p_sfb_offset = p_stream->sfb_offset_short_window;
    num_swb = p_stream->num_swb_short;
  } else {
    tns_max_bands = p_stream->tns_max_bands_long;
    tns_max_order = p_stream->tns_max_order_long;
    p_sfb_offset = p_stream->sfb_offset_long_window;
    num_swb = p_stream->num_swb_long;
  }

  for (w = 0; w < p_stream->num_windows; w++) {
    bottom = num_swb;

    for (f = 0; f < p_stream->n_filt[w]; f++) {
      p_data->m_order[w][f] = 0;
      top = bottom;
      bottom = MAX(top - p_stream->length[w][f], 0);
      tns_order = MIN(p_stream->order[w][f], tns_max_order);

      if (0 == tns_order)
        continue;

      tns_decode_coef(tns_order, p_stream->coef_res[w] + 3,
                      p_stream->coef_compress[w][f], p_stream->coef[w][f],
                      p_data->m_lpc[w][f], &(p_data->m_lpc_scalef[w][f]));

      min_sfb = MIN(tns_max_bands, p_stream->max_sfb);

      start = p_sfb_offset[MIN(bottom, min_sfb)];
      end = p_sfb_offset[MIN(top, min_sfb)];

      if ((size = end - start) <= 0)
        continue;

      p_data->m_order[w][f] = tns_order;
      p_data->m_size[w][f] = size;

      if (p_stream->direction[w][f]) {
        p_data->m_inc[w][f] = -1;
        p_data->m_start[w][f] = end - 1;
      } else {
        p_data->m_inc[w][f] = 1;
        p_data->m_start[w][f] = start;
      }

    }
    p_data->m_n_filt[w] = p_stream->n_filt[w];
  }

  p_data->m_num_windows = p_stream->num_windows;
}

/********************************************************************/

static const int tnsTable[2][16] =
{
  {   -734482665, -1380375881, -1859775393, -2114858546,
     -2114858546, -1859775393, -1380375881,  -734482665,
               0,   931758235,  1678970324,  2093641749,
      2093641749,  1678970324,   931758235,           0 },
   { -2138322861, -2065504841, -1922348530, -1713728946,
     -1446750378, -1130504462,  -775760571,  -394599085,
               0,   446486956,   873460290,  1262259218,
      1595891361,  1859775393,  2042378317,  2135719508 }
};

static const int tnsShift[] =
{
  0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
};

/********************************************************************/

static void tns_decode_coef(int order,
                            int coef_res_bits,
                            int coef_compress,
                            int *coef,
                            Ipp32s *a,
                            Ipp32s *scale)
{
  int i, m;
  int coef_res2;
  int tmp2[TNS_MAX_ORDER + 1];
  Ipp64s la[TNS_MAX_ORDER + 1];
  Ipp64s max;
  int shift, round, imax, scalef;

  /* Inverse quantization */

  coef_res2 = coef_res_bits - coef_compress;
  for (i = 0; i < order; i++) {
    int ind = ((coef[i] << (32 - coef_res2)) >> (32 - coef_res2)) + 8;
    tmp2[i] = tnsTable[coef_res_bits - 3][ind];
  }

 /* Conversion to LPC coefficients */

  shift = tnsShift[order] + 1;

  round = 0;
  if (shift > 0) {
    round = 1 << (shift - 1);
  }

  for (m = 1; m <= order; m++) {
    Ipp64s tmp = (Ipp64s)tmp2[m - 1];
    la[m] = tmp;
    for (i = 1; i <= (m >> 1); i++) {
      Ipp64s tmp0 = la[i];
      Ipp64s tmp1 = la[m - i];
      Ipp32s itmp0 = (Ipp32s)((tmp0 + round) >> shift);
      Ipp32s itmp1 = (Ipp32s)((tmp1 + round) >> shift);

      la[i]     = tmp0 + ((tmp * itmp1) >> (31 - shift));
      la[m - i] = tmp1 + ((tmp * itmp0) >> (31 - shift));
    }
  }

  max = 0;

  for (m = 1; m <= order; m++) {
    Ipp64s tmp0 = la[m];

    if (tmp0 < 0) tmp0 = -tmp0;
    if (max < tmp0) max = tmp0;
  }

  imax = (int)(max >> 31);
  scalef = 31;

  while (imax > 0) {
    imax >>= 1;
    scalef--;
  }

  a[0] = 1;
  for (m = 1; m <= order; m++) {
    a[m] = (Ipp32s)(la[m] >> (31 - scalef));
  }

  *scale = scalef;
}

/********************************************************************/

static void tns_ar_filter_plus(Ipp32s *p_spectrum,
                               int    size,
                               Ipp32s *lpc_coef,
                               int    scalef,
                               int    order)
{
  int    i, j;
  Ipp64s y;
  Ipp64s tmp;
  int    size0;

  size0 = order;
  if (size < order) size0 = size;

  for (i = 0; i < size0; i++) {
    y = ((Ipp64s)(*p_spectrum)) << scalef;

    for (j = 0; j < i; j++) {
      tmp = (Ipp64s)lpc_coef[j + 1] * p_spectrum[-j-1];
      y -= tmp;
    }
    *p_spectrum = (Ipp32s)(y >> scalef);
    p_spectrum++;
  }

  for (i = order; i < size; i++) {
    y = ((Ipp64s)(*p_spectrum)) << scalef;

    for (j = 0; j < order; j++) {
      tmp = (Ipp64s)lpc_coef[j + 1] * p_spectrum[-j-1];
      y -= tmp;
    }
    *p_spectrum = (Ipp32s)(y >> scalef);
    p_spectrum++;
  }
}

/********************************************************************/

static void tns_ar_filter_minus(Ipp32s *p_spectrum,
                                int size,
                                Ipp32s *lpc_coef,
                                int    scalef,
                                int order)
{
  int    i, j;
  Ipp64s y;
  Ipp64s tmp;
  int    size0;

  size0 = order;
  if (size < order) size0 = size;

  for (i = 0; i < size0; i++) {
    y = ((Ipp64s)(*p_spectrum)) << scalef;

    for (j = 0; j < i; j++) {
      tmp = (Ipp64s)lpc_coef[j + 1] * p_spectrum[j + 1];
      y -= tmp;
    }
    *p_spectrum = (Ipp32s)(y >> scalef);
    p_spectrum--;
  }

  for (i = order; i < size; i++) {
    y = ((Ipp64s)(*p_spectrum)) << scalef;

    for (j = 0; j < order; j++) {
      tmp = (Ipp64s)lpc_coef[j + 1] * p_spectrum[j + 1];
      y -= tmp;
    }
    *p_spectrum = (Ipp32s)(y >> scalef);
    p_spectrum--;
  }
}

/********************************************************************/

static void tns_ma_filter(Ipp32s *p_spectrum,
                          int size,
                          int inc,
                          Ipp32s *lpc_coef,
                          int    scalef,
                          int order)
{
  int     i, j;
  Ipp64s   y;
  Ipp32s   flt_state[TNS_MAX_ORDER+1024];
  Ipp32s   *p_state = flt_state + size;
  Ipp64s   tmp;

  for (i = 0; i < order; i++)
    p_state[i] = 0;

  for (i = 0; i < size; i++) {
    y = ((Ipp64s)(*p_spectrum)) << scalef;

    for (j = 0; j < order; j++) {
      tmp = (Ipp64s)lpc_coef[j + 1] * p_state[j];
      y += tmp;
    }

    p_state--;
    p_state[0] = *p_spectrum;
    *p_spectrum = (Ipp32s)(y >> scalef);
    p_spectrum += inc;
  }
}

/********************************************************************/
