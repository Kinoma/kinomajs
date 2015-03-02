/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#include "arch.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL 0
#endif

int kinoma_speex_init(int imp );

spx_word32_t inner_prod_len40_arm_v6(const spx_word16_t *x, const spx_word16_t *y);
void filter_mem16_N40_ord10_arm_v6(const spx_word16_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_word16_t *y, spx_mem_t *mem);
void iir_mem16_N40_ord10_arm_v6(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, spx_mem_t *mem);

spx_word32_t inner_prod_arm_v6(const spx_word16_t *x, const spx_word16_t *y, int len);
void filter_mem16_arm_v6(const spx_word16_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack);
void iir_mem16_arm_v6(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack);
int pitch_gain_search_3tap_vq_arm_v6(const signed char *ptr, int gain_cdbk_size, spx_word16_t *C16, spx_word16_t max_gain);
void vq_nbest_arm_v6(spx_word16_t *in, const spx_word16_t *codebook, int len, int entries, spx_word32_t *E, int N, int *nbest, spx_word32_t *best_dist, char *stack);

int pitch_gain_search_3tap_vq(const signed char *ptr, int gain_cdbk_size, spx_word16_t *C16, spx_word16_t max_gain);

extern spx_word32_t (*inner_prod_universal )				(const spx_word16_t *x, const spx_word16_t *y, int len);
extern void			(*filter_mem16_universal )				(const spx_word16_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack);
extern void			(*iir_mem16_universal )					(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack);
extern int			(*pitch_gain_search_3tap_vq_universal )	(const signed char *ptr, int gain_cdbk_size, spx_word16_t *C16, spx_word16_t max_gain);
extern void			(*vq_nbest_universal )					(spx_word16_t *in, const spx_word16_t *codebook, int len, int entries, spx_word32_t *E, int N, int *nbest, spx_word32_t *best_dist, char *stack);

#ifdef KINOMA_ARM_OPTIMIZATION

#define inner_prod_x					(*inner_prod_universal)
#define filter_mem16_x				(*filter_mem16_universal)
#define iir_mem16_x					(*iir_mem16_universal)
#define pitch_gain_search_3tap_vq_x	(*pitch_gain_search_3tap_vq_universal)
#define vq_nbest_x					(*vq_nbest_universal)

#else

#define inner_prod_x					inner_prod
#define filter_mem16_x				filter_mem16
#define iir_mem16_x					iir_mem16
#define pitch_gain_search_3tap_vq_x	pitch_gain_search_3tap_vq
#define vq_nbest_x					vq_nbest

#endif

#ifdef __cplusplus
}
#endif
