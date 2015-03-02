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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Fsk.h"
#include "FskPlatformImplementation.h"

#include "kinoma_speex.h"
#include "FskArch.h"
#include "ltp.h"
#include "filters.h"
#include "vq.h"

spx_word32_t	(*inner_prod_universal )				(const spx_word16_t *x, const spx_word16_t *y, int len)=NULL;
void			(*filter_mem16_universal )				(const spx_word16_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack)=NULL;
void			(*iir_mem16_universal )					(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack)=NULL;
int				(*pitch_gain_search_3tap_vq_universal )	(const signed char *ptr, int gain_cdbk_size, spx_word16_t *C16, spx_word16_t max_gain)=NULL;
void			(*vq_nbest_universal )					(spx_word16_t *in, const spx_word16_t *codebook, int len, int entries, spx_word32_t *E, int N, int *nbest, spx_word32_t *best_dist, char *stack)=NULL;


int kinoma_speex_init(int implementation )
{
	int imp_out = FSK_ARCH_C;

	if( implementation == FSK_ARCH_AUTO )
		implementation = FskHardwareGetARMCPU_All();

	if( implementation > FSK_ARCH_ARM_V6 )
		implementation = FSK_ARCH_ARM_V6; //our highest optimization

	switch( implementation )
	{
		case FSK_ARCH_ARM_V6:
#ifdef KINOMA_ARM_OPTIMIZATION
			inner_prod_universal  = inner_prod_arm_v6;
			filter_mem16_universal = filter_mem16_arm_v6;
			iir_mem16_universal	  = iir_mem16_arm_v6;
			vq_nbest_universal	  = vq_nbest_arm_v6;
			pitch_gain_search_3tap_vq_universal =pitch_gain_search_3tap_vq_arm_v6;
			
			imp_out = FSK_ARCH_ARM_V6;
			break;
#endif
		case FSK_ARCH_ARM_V5:
		case FSK_ARCH_XSCALE:
		case FSK_ARCH_C:
		default:
			inner_prod_universal  = inner_prod;
			filter_mem16_universal = filter_mem16;
			iir_mem16_universal	  = iir_mem16;
			vq_nbest_universal	  = vq_nbest;
			pitch_gain_search_3tap_vq_universal =pitch_gain_search_3tap_vq;

			imp_out = FSK_ARCH_C;
			break;
	}

	return imp_out;
}
