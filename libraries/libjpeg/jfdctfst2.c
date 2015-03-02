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
/*
 * This file is used for extending the arm optimization aiming at the jpeg encoding process
 * The fdct standard C implementation can be found in jfdctfst.c
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */

#include "FskArch.h"

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gFskJpegDctTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskJpegDct"};
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gFskJpegDctTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif

/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif

void (*jpeg_fdct_ifast_universal)(DCTELEM *data) = NULL;
void (*jpeg_quantize_universal) (JCOEFPTR coef_block, DCTELEM * divisors, DCTELEM * workspace) = NULL;
void (*jpeg_convsamp_universal ) (JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM * workspace) = NULL;

#ifdef DCT_IFAST_ARM_V7_SUPPORTED
extern void jpeg_fdct_ifast_arm_v7(DCTELEM *data);
extern void jpeg_quantize_arm_v7(JCOEFPTR coef_block, DCTELEM *divisors, DCTELEM *workspace);
extern void jpeg_convsamp_arm_v7(JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM *workspace);
#endif

int config_ifast_dct(int implementation)
{
	int imp_out = FSK_ARCH_C;

	if(implementation == FSK_ARCH_AUTO)
		implementation = FskHardwareGetARMCPU_All();

	switch(implementation) {
#ifdef DCT_IFAST_ARM_V7_SUPPORTED
		case FSK_ARCH_ARM_V5:
		case FSK_ARCH_XSCALE:
		case FSK_ARCH_ARM_V6:
			jpeg_fdct_ifast_universal = jpeg_fdct_ifast;
			jpeg_quantize_universal = jpeg_quantize;
			jpeg_convsamp_universal = jpeg_convsamp;
			imp_out = FSK_ARCH_ARM_V6;
			break;
		case FSK_ARCH_ARM_V7:
			jpeg_fdct_ifast_universal = jpeg_fdct_ifast_arm_v7;
			jpeg_quantize_universal = jpeg_quantize_arm_v7;
			jpeg_convsamp_universal = jpeg_convsamp_arm_v7;
			imp_out = FSK_ARCH_ARM_V7;
			break;
#endif
#ifdef DCT_IFAST_C_SUPPORTED
		case FSK_ARCH_C:
		default:
			jpeg_fdct_ifast_universal = jpeg_fdct_ifast;
			jpeg_quantize_universal = jpeg_quantize;
			jpeg_convsamp_universal = jpeg_convsamp;
			imp_out = FSK_ARCH_C;
			break;
#endif
	}

	return imp_out;
}

