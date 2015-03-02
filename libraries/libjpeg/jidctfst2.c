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
/* this file is derived from jidctfst.c
8x8, 1x1 generate the same result with optimizatin on clipping
4x4, 2x2 are derived from fast 8x8 with further simpification and optimization
their counterpart arm impementation can be found in jidctfst.arm.s
	--bnie	5/6/2008
*/

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */


#include "FskPlatformImplementation.h"
#include "FskArch.h"


#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gFskJpegIdctTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskJpegIdct"};
//FskInstrumentedSimpleType(FskJpegIdct, FskJpegIdct);
#define dlog(...)  do { FskInstrumentedTypePrintfDebug  (&gFskJpegIdctTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define dlog(...)
#endif

/*
#if TARGET_OS_ANDROID
//#ifdef SUPPORT_NEON

#include <pthread.h>
#include "cpu-features.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#endif
*/

//borrowed from kinoma_ipp_lib.h
//#define FSK_ARCH_AUTO			0
//#define FSK_ARCH_C				1
//#define FSK_ARCH_ARM_V4		2
//#define FSK_ARCH_ARM_V5		3
//#define FSK_ARCH_XSCALE		4
//#define FSK_ARCH_ARM_V6		5
//#define FSK_ARCH_ARM_V7		6

void jpeg_idct_2x2_arm_v6(int w12, int w34, int wk, int quant, int coef, int out0, int out1);
void jpeg_idct_4x4_arm_v6(int w12, int w34, int wk, int quant, int coef, int out0, int out1, int out2, int out3);

/*
 * This module is specialized to the case DCTSIZE = 8.
 */
#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif

#define w1  ((INT32)  277)		/* FIX(1.082392200) */
#define w2  ((INT32)  362)		/* FIX(1.414213562) */
#define w3  ((INT32)  473)		/* FIX(1.847759065) */
#define w4  ((INT32)  -669)		/* FIX(2.613125930) */

/* Dequantize a coefficient by multiplying it by the multiplier-table
 * entry; produce a DCTELEM result.  For 8-bit data a 16x16->16
 * multiplication will do.  For 12-bit data, the multiplier table is
 * declared INT32, so a 32-bit multiply will be used.
 */

#define TMP_TYPE	short	//***is it enough?  bnie --4/26/08
//#define TMP_TYPE	int	//***is it enough?  bnie --4/26/08
#define CLIP(val, limit) if (((unsigned int) val) > (unsigned int) limit) val = limit & ~(((signed int) val) >> 31)
#define add( a, b, c )		a = b + c
#define sub( a, b, c )		a = b - c
#define mul( a, b, c )		a = b * c
#define addrs( a, b, c, d )	a = b + (c>>d)
#define rsbrs( a, b, c, d )	a = (c>>d) - b

void jpeg_idct_8x8_arm_v5( int w12, int w34, short *wk, short *coef,  short *quant, short *out0, short *out1, short *out2, short *out3,	short *out4, short *out5, short *out6, short *out7);
void jpeg_idct_4x4_arm_v5( int w12, int w34, short *wk, short *coef,  short *quant, short *out0, short *out1, short *out2, short *out3  );
void jpeg_idct_2x2_arm_v5( int w12, int w34, short *wk, short *coef,  short *quant, short *out0, short *out1);
void jpeg_idct_8x8_arm_v7(void * dct_table, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
void jpeg_idct_4x4_arm_v7(void * dct_table, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);
void jpeg_idct_2x2_arm_v7(void * dct_table, JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col);


int cap_neon_ycc_rgb (void)
{
  if(   (BITS_IN_JSAMPLE != 8)                          ||
        (sizeof(JDIMENSION) != 4)                       ||
        ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4)))
    return 0;

    return 1;
}

#ifdef IDCT_IFAST_8x8_C_SUPPORTED

//void IDCT_COL_8( JCOEFPTR coef, IFAST_MULT_TYPE * quant, TMP_TYPE *wk)
#define IDCT_COL_8( coef, quant, wk, offset )												\
{																							\
	if (coef[8*1+offset] == 0 && coef[8*2+offset] == 0 &&									\
		coef[8*3+offset] == 0 && coef[8*4+offset] == 0 &&									\
		coef[8*5+offset] == 0 && coef[8*6+offset] == 0 &&									\
		coef[8*7+offset] == 0)																\
	{																						\
		/* AC terms all zero */																\
		int dc = coef[8*0+offset] * quant[8*0+offset];										\
																							\
		wk[8*0+offset] = dc;																\
		wk[8*1+offset] = dc;																\
		wk[8*2+offset] = dc;																\
		wk[8*3+offset] = dc;																\
		wk[8*4+offset] = dc;																\
		wk[8*5+offset] = dc;																\
		wk[8*6+offset] = dc;																\
		wk[8*7+offset] = dc;																\
	}																						\
	else																					\
	{   /* Even part */																		\
		int q0, q1, q2, q3, q4, q5, q6, q7, qq;											\
																							\
		q0 = coef[8*0+offset] * quant[8*0+offset];											\
		q1 = coef[8*1+offset] * quant[8*1+offset];											\
		q2 = coef[8*2+offset] * quant[8*2+offset];											\
		q3 = coef[8*3+offset] * quant[8*3+offset];											\
		q4 = coef[8*4+offset] * quant[8*4+offset];											\
		q5 = coef[8*5+offset] * quant[8*5+offset];											\
		q6 = coef[8*6+offset] * quant[8*6+offset];											\
		q7 = coef[8*7+offset] * quant[8*7+offset];											\
																							\
		add( qq, q0, q4 );																	\
		sub( q0, q0, q4 );																	\
		add( q4, q2, q6 );																	\
		sub( q6, q2, q6 );																	\
		mul( q6, w2, q6 );																	\
		rsbrs( q6, q4, q6, 8 );																\
																							\
		add( q2, q0, q6 );																	\
		sub( q0, q0, q6 );																	\
																							\
		add( q6, qq, q4 );																	\
		sub( q4, qq, q4 );																	\
																							\
		/* Odd part */																		\
		add( qq, q5, q3 );																	\
		sub( q3, q5, q3 );																	\
		add( q5, q1, q7 );																	\
		sub( q7, q1, q7 );																	\
		add( q1, q5, qq );																	\
		sub( q5, q5, qq );																	\
		mul( q5, w2, q5 );																	\
																							\
		add( qq, q3, q7 );																	\
		mul( qq, w3, qq );																	\
		mul( q7, w1, q7 );																	\
		sub( q7, q7, qq );																	\
		mul( q3, w4, q3 );																	\
		add( q3, q3, qq );																	\
												 											\
		rsbrs( q3, q1, q3, 8 );																\
		rsbrs( q5, q3, q5, 8 );																\
		addrs( q7, q5, q7, 8 );																\
																							\
		wk[8*0+offset] = q6+q1;																\
		wk[8*1+offset] = q2+q3;																\
		wk[8*2+offset] = q0+q5;																\
		wk[8*3+offset] = q4-q7;																\
		wk[8*4+offset] = q4+q7;																\
		wk[8*5+offset] = q0-q5;																\
		wk[8*6+offset] = q2-q3;																\
		wk[8*7+offset] = q6-q1;																\
	}																						\
}

//void IDCT_ROW_8( int CONST_255, TMP_TYPE *wk, JSAMPROW  outptr )								
#define IDCT_ROW_8( CONST_255, wk, outptr, offset )											\
{																							\
/*#ifndef NO_ZERO_ROW_TEST*/																\
	if (wk[1+offset] == 0 && wk[2+offset] == 0 &&											\
		wk[3+offset] == 0 && wk[4+offset] == 0 &&											\
		wk[5+offset] == 0 && wk[6+offset] == 0 &&											\
		wk[7+offset] == 0)																	\
	{																						\
		/* AC terms all zero */																\
		int dc = wk[0+offset];																		\
																							\
		dc = 128 + (dc>>5);																	\
		CLIP(dc, CONST_255);																\
																							\
		outptr[0] = dc;																\
		outptr[1] = dc;																\
		outptr[2] = dc;																\
		outptr[3] = dc;																\
		outptr[4] = dc;																\
		outptr[5] = dc;																\
		outptr[6] = dc;																\
		outptr[7] = dc;																\
	}																						\
	else																					\
/*#endif*/																					\
	{																						\
		int q0, q1, q2, q3, q4, q5, q6, q7, qq;												\
																							\
		/* Even part */																		\
		q0 = wk[0+offset];																			\
		q1 = wk[1+offset];																			\
		q2 = wk[2+offset];																			\
		q3 = wk[3+offset];																			\
		q4 = wk[4+offset];																			\
		q5 = wk[5+offset];																			\
		q6 = wk[6+offset];																			\
		q7 = wk[7+offset];																			\
																							\
		add( qq, q0, q4 );																	\
		sub( q0, q0, q4 );																	\
		add( q4, q2, q6 );																	\
		sub( q6, q2, q6 );																	\
		mul( q6, w2, q6 );																	\
		rsbrs( q6, q4, q6, 8 );																\
																							\
		add( q2, q0, q6 );																	\
		sub( q0, q0, q6 );																	\
																							\
		add( q6, qq, q4 );																	\
		sub( q4, qq, q4 );																	\
																							\
		/* Odd part */																		\
		add( qq, q5, q3 );																	\
		sub( q3, q5, q3 );																	\
		add( q5, q1, q7 );																	\
		sub( q7, q1, q7 );																	\
		add( q1, q5, qq );																	\
		sub( q5, q5, qq );																	\
		mul( q5, w2, q5 );																	\
																							\
		add( qq, q3, q7 );																	\
		mul( qq, w3, qq );																	\
		mul( q7, w1, q7 );																	\
		sub( q7, q7, qq );																	\
		mul( q3, w4, q3 );																	\
		add( q3, q3, qq );																	\
												 											\
		rsbrs( q3, q1, q3, 8 );																\
		rsbrs( q5, q3, q5, 8 );																\
		addrs( q7, q5, q7, 8 );																\
																							\
		{																					\
			int t;																			\
																							\
			t= 128 + ((q6+q1)>>5); CLIP(t,CONST_255); outptr[0] =t;							\
			t= 128 + ((q2+q3)>>5); CLIP(t,CONST_255); outptr[1] =t;							\
			t= 128 + ((q0+q5)>>5); CLIP(t,CONST_255); outptr[2] =t;							\
			t= 128 + ((q4-q7)>>5); CLIP(t,CONST_255); outptr[3] =t;							\
			t= 128 + ((q4+q7)>>5); CLIP(t,CONST_255); outptr[4] =t;							\
			t= 128 + ((q0-q5)>>5); CLIP(t,CONST_255); outptr[5] =t;							\
			t= 128 + ((q2-q3)>>5); CLIP(t,CONST_255); outptr[6] =t;							\
			t= 128 + ((q6-q1)>>5); CLIP(t,CONST_255); outptr[7] =t;							\
		}																					\
	}																						\
}


void jpeg_idct_ifast_8x8_c														
(																				
	j_decompress_ptr	cinfo,													
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	JSAMPROW		out2 = output_buf[2] + output_col;
	JSAMPROW		out3 = output_buf[3] + output_col;
	JSAMPROW		out4 = output_buf[4] + output_col;
	JSAMPROW		out5 = output_buf[5] + output_col;
	JSAMPROW		out6 = output_buf[6] + output_col;
	JSAMPROW		out7 = output_buf[7] + output_col;
	TMP_TYPE		wk[64];	/* buffers data between passes */
	int				CONST_255 = 255;

	IDCT_COL_8(coef, quant, wk, 0 );
	IDCT_COL_8(coef, quant, wk, 1 );
	IDCT_COL_8(coef, quant, wk, 2 );
	IDCT_COL_8(coef, quant, wk, 3 );
	IDCT_COL_8(coef, quant, wk, 4 );
	IDCT_COL_8(coef, quant, wk, 5 );
	IDCT_COL_8(coef, quant, wk, 6 );
	IDCT_COL_8(coef, quant, wk, 7 );

	IDCT_ROW_8( CONST_255, wk, out0, 0*8 );
	IDCT_ROW_8( CONST_255, wk, out1, 1*8 );
	IDCT_ROW_8( CONST_255, wk, out2, 2*8 );
	IDCT_ROW_8( CONST_255, wk, out3, 3*8 );
	IDCT_ROW_8( CONST_255, wk, out4, 4*8 );
	IDCT_ROW_8( CONST_255, wk, out5, 5*8 );
	IDCT_ROW_8( CONST_255, wk, out6, 6*8 );
	IDCT_ROW_8( CONST_255, wk, out7, 7*8 );
}


#endif

#ifdef IDCT_IFAST_SCALING_C_SUPPORTED

//void IDCT_COL_4( JCOEFPTR col, IFAST_MULT_TYPE * quant, TMP_TYPE *wsptr)
#define IDCT_COL_4( col, quant, wsptr, offset )									\
{																				\
	if (col[8*1+offset] == 0 && col[8*2+offset] == 0 &&	col[8*3+offset] == 0 &&	\
		col[8*5+offset] == 0 && col[8*6+offset] == 0 &&	col[8*7+offset] == 0)	\
	{																			\
		/* AC terms all zero */													\
		int dc = col[8*0+offset] * quant[8*0+offset];							\
																				\
		wsptr[8*0+offset] = dc;													\
		wsptr[8*1+offset] = dc;													\
		wsptr[8*2+offset] = dc;													\
		wsptr[8*3+offset] = dc;													\
	}																			\
	else																		\
	{   /* Even part */															\
		int q0, q1, q2, q3, q5, q6, q7;											\
																				\
		q0 = col[8*0+offset] * quant[8*0+offset];								\
		q2 = col[8*2+offset] * quant[8*2+offset];								\
		q6 = col[8*6+offset] * quant[8*6+offset];								\
		q1 = col[8*1+offset] * quant[8*1+offset];								\
		q3 = col[8*3+offset] * quant[8*3+offset];								\
		q5 = col[8*5+offset] * quant[8*5+offset];								\
		q7 = col[8*7+offset] * quant[8*7+offset];								\
																				\
		q0 = q0<<9;																\
		sub( q2, q2, q6 );														\
		mul( q2, w2, q2 );														\
		add( q6, q0, q2 );														\
		sub( q2, q0, q2 );														\
																				\
		/* Odd part */															\
		sub( q5, q5, q3 );														\
		sub( q1, q1, q7 );														\
		add( q3, q5, q1 );														\
		mul( q3, w3, q3 );														\
		mul( q1, w1, q1 );														\
		sub( q1, q1, q3 );														\
		mul( q5, w4, q5 );														\
		add( q5, q5, q3 );														\
																				\
		wsptr[8*0+offset] = (q6 + q5)>>9;										\
		wsptr[8*1+offset] = (q2 - q1)>>9;										\
		wsptr[8*2+offset] = (q2 + q1)>>9;										\
		wsptr[8*3+offset] = (q6 - q5)>>9;										\
	}																			\
}

//void IDCT_ROW_4( int CONST_255, JSAMPROW  outptr, TMP_TYPE *row)
#define IDCT_ROW_4( CONST_255, outptr, row, offset)								\
{																				\
/*#ifndef NO_ZERO_ROW_TEST*/													\
	if (row[1+offset] == 0 && row[2+offset] == 0 && row[3+offset] == 0 &&		\
		row[5+offset] == 0 && row[6+offset] == 0 && row[7+offset] == 0)			\
	{																			\
		/* AC terms all zero */													\
		int dc = row[0+offset];													\
		dc = 128 + (dc>>5);														\
		CLIP(dc, CONST_255);													\
		outptr[0] = dc;															\
		outptr[1] = dc;															\
		outptr[2] = dc;															\
		outptr[3] = dc;															\
	}																			\
	else																		\
/*#endif*/																		\
	{																			\
		int q0, q1, q2, q3, q5, q6, q7;											\
																				\
		/* Even part */															\
		q0 = row[0+offset];														\
		q2 = row[2+offset];														\
		q6 = row[6+offset];														\
		q1 = row[1+offset];														\
		q3 = row[3+offset];														\
		q5 = row[5+offset];														\
		q7 = row[7+offset];														\
																				\
		q0 = q0<<9;																\
		sub( q2, q2, q6 );														\
		mul( q2, w2, q2 );														\
		add( q6, q0, q2 );														\
		sub( q2, q0, q2 );														\
																				\
		/* Odd part */															\
		sub( q5, q5, q3 );														\
		sub( q1, q1, q7 );														\
		add( q3, q5, q1 );														\
		mul( q3, w3, q3 );														\
		mul( q1, w1, q1 );														\
		sub( q1, q1, q3 );														\
		mul( q5, w4, q5 );														\
		add( q5, q5, q3 );														\
																				\
		{																		\
			int t;																\
																				\
			t= 128 + ((q6+q5)>>14); CLIP(t,CONST_255); outptr[0] =t;			\
			t= 128 + ((q2-q1)>>14); CLIP(t,CONST_255); outptr[1] =t;			\
			t= 128 + ((q2+q1)>>14); CLIP(t,CONST_255); outptr[2] =t;			\
			t= 128 + ((q6-q5)>>14); CLIP(t,CONST_255); outptr[3] =t;			\
		}																		\
	}																			\
}

void jpeg_idct_ifast_4x4_c
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;	
	JSAMPROW		out1 = output_buf[1] + output_col;	
	JSAMPROW		out2 = output_buf[2] + output_col;	
	JSAMPROW		out3 = output_buf[3] + output_col;	
	TMP_TYPE		wk[32];	/* buffers data between passes */
	int				CONST_255 = 255;

	IDCT_COL_4(coef, quant, wk, 0 );
	IDCT_COL_4(coef, quant, wk, 1 );
	IDCT_COL_4(coef, quant, wk, 2 );
	IDCT_COL_4(coef, quant, wk, 3 );
	IDCT_COL_4(coef, quant, wk, 5 );
	IDCT_COL_4(coef, quant, wk, 6 );
	IDCT_COL_4(coef, quant, wk, 7 );

	IDCT_ROW_4( CONST_255, out0, wk, 0 );
	IDCT_ROW_4( CONST_255, out1, wk, 8 );
	IDCT_ROW_4( CONST_255, out2, wk, 16);
	IDCT_ROW_4( CONST_255, out3, wk, 24);
}

#define IDCT_COL_2( col, quant, wsptr, offset)			\
{														\
	if (col[8*1+offset] == 0 && col[8*3+offset] == 0 &&	\
		col[8*5+offset] == 0 && col[8*7+offset] == 0)	\
	{													\
		int dc = col[8*0+offset] * quant[8*0+offset];	\
														\
		wsptr[8*0+offset] = dc;							\
		wsptr[8*1+offset] = dc;							\
	}													\
	else												\
	{   												\
		int q0, q1, q3, q5, q7;							\
														\
		q0 = col[8*0+offset] * quant[8*0+offset];		\
		q1 = col[8*1+offset] * quant[8*1+offset];		\
		q3 = col[8*3+offset] * quant[8*3+offset];		\
		q5 = col[8*5+offset] * quant[8*5+offset];		\
		q7 = col[8*7+offset] * quant[8*7+offset];		\
														\
		sub( q5, q5, q3 );								\
		sub( q1, q1, q7 );								\
		add( q3, q5, q1 );								\
		mul( q3, w3, q3 );								\
		mul( q1, w1, q1 );								\
		mul( q5, w4, q5 );								\
		sub( q1, q1, q5 );								\
		sub( q1, q1, (q3<<1));							\
														\
		wsptr[8*0+offset] = q0-(q1>>10);				\
		wsptr[8*1+offset] = q0+(q1>>10);				\
	}													\
}

#define IDCT_ROW_2( CONST_255, outptr, row, offset )	\
{														\
/*#ifndef NO_ZERO_ROW_TEST*/							\
	if (row[1+offset] == 0 && row[3+offset] == 0 &&		\
		row[5+offset] == 0 && row[7+offset] == 0)		\
	{													\
		int dc = row[0+offset];							\
		dc = 128 + (dc>>5);								\
		CLIP(dc, CONST_255);							\
		outptr[0] = dc;									\
		outptr[1] = dc;									\
	}													\
	else												\
/*#endif*/												\
	{													\
		int q0, q1, q3, q5, q7;							\
														\
		q0 = row[0+offset];								\
		q1 = row[1+offset];								\
		q3 = row[3+offset];								\
		q5 = row[5+offset];								\
		q7 = row[7+offset];								\
														\
		sub( q5, q5, q3 );								\
		sub( q1, q1, q7 );								\
														\
		add( q3, q5, q1 );								\
		mul( q3, w3, q3 );								\
		mul( q1, w1, q1 );								\
		mul( q5, w4, q5 );								\
		sub( q1, q1, q5 );								\
		sub( q1, q1, (q3<<1));							\
														\
		{												\
			int t;										\
														\
			t= 128 + ((q0-(q1>>10))>>5); CLIP(t,CONST_255); outptr[0] =t;	\
			t= 128 + ((q0+(q1>>10))>>5); CLIP(t,CONST_255); outptr[1] =t;	\
		}												\
	}													\
}

void jpeg_idct_ifast_2x2_c
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	TMP_TYPE		wk[16];	/* buffers data between passes */
	int				CONST_255 = 255;

	IDCT_COL_2(coef, quant, wk, 0);
	IDCT_COL_2(coef, quant, wk, 1)
	IDCT_COL_2(coef, quant, wk, 3)
	IDCT_COL_2(coef, quant, wk, 5)
	IDCT_COL_2(coef, quant, wk, 7)

	IDCT_ROW_2( CONST_255, out0, wk, 0 );		
	IDCT_ROW_2( CONST_255, out1, wk, 8 );		
}

void jpeg_idct_ifast_1x1_c
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	int				CONST_255 = 255;
	int				t;

	t = coef[0] * quant[0];;										
	t = 128 + (t>>5);
	CLIP(t,CONST_255); 
	output_buf[0][output_col] =t;	
}
#endif

#ifdef IDCT_IFAST_8x8_ARM_V6_SUPPORTED

void jpeg_idct_ifast_8x8_arm_v5														
(																				
	j_decompress_ptr	cinfo,													
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	JSAMPROW		out2 = output_buf[2] + output_col;
	JSAMPROW		out3 = output_buf[3] + output_col;
	JSAMPROW		out4 = output_buf[4] + output_col;
	JSAMPROW		out5 = output_buf[5] + output_col;
	JSAMPROW		out6 = output_buf[6] + output_col;
	JSAMPROW		out7 = output_buf[7] + output_col;
	TMP_TYPE		wk[64];
	int w12 = (w1<<16)|(w2&0x0000ffff);
	int w34 = (w3<<16)|(w4&0x0000ffff);

	dlog( "calling jpeg_idct_ifast_8x8_arm_v5\n");
	jpeg_idct_8x8_arm_v5(w12, w34, wk, quant, coef, (short *)out0, (short *)out1, (short *)out2, (short *)out3, (short *)out4, (short *)out5, (short *)out6, (short *)out7 );
}

void jpeg_idct_ifast_8x8_arm_v7
(																				
	j_decompress_ptr	cinfo,													
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	dlog( "calling jpeg_idct_8x8_arm_v7\n");
	jpeg_idct_8x8_arm_v7(compptr->dct_table, coef, output_buf, output_col);
}

//***bnie: for win32 debug only, very very slow...
/*
void jpeg_idct_add_sat_c(short *src, unsigned char *dst, int const_128_128 )
{
	int i;

	for( i = 0; i < 8; i++ ) 
		dst[i] = src[0*8+i]+0x80>255? 255 : src[0*8+i]+0x80<0? 0 : src[0*8+i]+0x80;
}

void jpeg_idct_ifast_8x8_openmax_c														
(																				
	j_decompress_ptr	cinfo,													
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	JSAMPROW		out2 = output_buf[2] + output_col;
	JSAMPROW		out3 = output_buf[3] + output_col;
	JSAMPROW		out4 = output_buf[4] + output_col;
	JSAMPROW		out5 = output_buf[5] + output_col;
	JSAMPROW		out6 = output_buf[6] + output_col;
	JSAMPROW		out7 = output_buf[7] + output_col;
	TMP_TYPE		wk[64];	//buffers data between passes
	int const_0x80_0x80 = (0x80<<16)|0x80;
	__ALIGN8(short, pDst, 64);

	omxICJP_DCTQuantInv_S16_kinoma_c( coef, pDst,compptr->dct_table2 );

	jpeg_idct_add_sat_c(&pDst[0*8], out0, const_0x80_0x80 );
	jpeg_idct_add_sat_c(&pDst[1*8], out1, const_0x80_0x80 );
	jpeg_idct_add_sat_c(&pDst[2*8], out2, const_0x80_0x80 );
	jpeg_idct_add_sat_c(&pDst[3*8], out3, const_0x80_0x80 );
	jpeg_idct_add_sat_c(&pDst[4*8], out4, const_0x80_0x80 );
	jpeg_idct_add_sat_c(&pDst[5*8], out5, const_0x80_0x80 );
	jpeg_idct_add_sat_c(&pDst[6*8], out6, const_0x80_0x80 );
	jpeg_idct_add_sat_c(&pDst[7*8], out7, const_0x80_0x80 );
}
*/

//***bnie: borrowed from kinoma_utilities.h
#if defined( WIN32) || defined( _WIN32_WCE )
#include <windows.h>

#define __ALIGN16(type, name, size)		__declspec(align(16)) type name[size]
#define __ALIGN8(type, name, size)		__declspec(align(8))  type name[size]
#define __ALIGN4(type, name, size)		__declspec(align(4))  type name[size]

#define DECL_ALIGN_16	__declspec(align(16))
#define DECL_ALIGN_8	__declspec(align(8))
#define DECL_ALIGN_4	__declspec(align(4))


#elif TARGET_OS_ANDROID || TARGET_OS_IPHONE

#define __ALIGN16(type, name, size)     type name[size] __attribute__ ((aligned (16)))
#define __ALIGN8(type, name, size)      type name[size] __attribute__ ((aligned (8)))
#define __ALIGN4(type, name, size)      type name[size] __attribute__ ((aligned (4)))

#define DECL_ALIGN_16   __attribute__ ((aligned (16)))
#define DECL_ALIGN_8    __attribute__ ((aligned (8)))
#define DECL_ALIGN_4    __attribute__ ((aligned (4)))

#define LARGE_INTEGER long long


#else
#define __ALIGN16(type, name, size) Ipp8u _a16_##name[(size)*sizeof(type)+15]; type *name = (type*)(((Ipp32s)(_a16_##name) + 15) & ~15)
#define __ALIGN8(type, name, size)  Ipp8u _a8_##name[(size)*sizeof(type)+7]; type *name = (type*)(((Ipp32s)(_a8_##name) + 7) & ~7)
#define __ALIGN4(type, name, size)  Ipp8u _a4_##name[(size)*sizeof(type)+3]; type *name = (type*)(((Ipp32s)(_a4_##name) + 3) & ~3)

#define DECL_ALIGN_16
#define DECL_ALIGN_8
#define DECL_ALIGN_4

#define LARGE_INTEGER long long

#endif

#ifdef SUPPORT_OPENMAX
void jpeg_idct_ifast_8x8_openmax_arm_v6														
(																				
	j_decompress_ptr	cinfo,													
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	JSAMPROW		out2 = output_buf[2] + output_col;
	JSAMPROW		out3 = output_buf[3] + output_col;
	JSAMPROW		out4 = output_buf[4] + output_col;
	JSAMPROW		out5 = output_buf[5] + output_col;
	JSAMPROW		out6 = output_buf[6] + output_col;
	JSAMPROW		out7 = output_buf[7] + output_col;
	__ALIGN8(unsigned char, pDst, 64);

	dlog( "calling jpeg_idct_ifast_8x8_openmax_arm_v6\n");
	omxICJP_DCTQuantInv_S16_kinoma( coef, pDst,compptr->dct_table2 );
	*(long *)(out0) = *(long *)(pDst+0*8+0); *(long *)(out0+4) = *(long *)(pDst+0*8+4);
	*(long *)(out1) = *(long *)(pDst+1*8+0); *(long *)(out1+4) = *(long *)(pDst+1*8+4);
	*(long *)(out2) = *(long *)(pDst+2*8+0); *(long *)(out2+4) = *(long *)(pDst+2*8+4);
	*(long *)(out3) = *(long *)(pDst+3*8+0); *(long *)(out3+4) = *(long *)(pDst+3*8+4);
	*(long *)(out4) = *(long *)(pDst+4*8+0); *(long *)(out4+4) = *(long *)(pDst+4*8+4);
	*(long *)(out5) = *(long *)(pDst+5*8+0); *(long *)(out5+4) = *(long *)(pDst+5*8+4);
	*(long *)(out6) = *(long *)(pDst+6*8+0); *(long *)(out6+4) = *(long *)(pDst+6*8+4);
	*(long *)(out7) = *(long *)(pDst+7*8+0); *(long *)(out7+4) = *(long *)(pDst+7*8+4);
}
#endif

#endif /* DCT_IFAST_8x8_ARM_V6_SUPPORTED */

#ifdef IDCT_IFAST_SCALING_ARM_V6_SUPPORTED

void jpeg_idct_ifast_4x4_arm_v5
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	JSAMPROW		out2 = output_buf[2] + output_col;
	JSAMPROW		out3 = output_buf[3] + output_col;
	TMP_TYPE		wk[32];
	int w12 = (w1<<16)|(w2&0x0000ffff);
	int w34 = (w3<<16)|(w4&0x0000ffff);

	dlog( "calling jpeg_idct_ifast_4x4_arm_v5\n");
	jpeg_idct_4x4_arm_v5(w12, w34, wk, quant, coef, (short *)out0, (short *)out1, (short *)out2, (short *)out3);
}

void jpeg_idct_ifast_2x2_arm_v5
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	TMP_TYPE		wk[32];
	int w12 = (w1<<16)|(w2&0x0000ffff);
	int w34 = (w3<<16)|(w4&0x0000ffff);

	dlog( "calling jpeg_idct_ifast_2x2_arm_v5\n");
	jpeg_idct_2x2_arm_v5(w12, w34, wk, quant, coef, (short *)out0, (short *)out1);
}

void jpeg_idct_ifast_1x1_arm_v5
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	int				CONST_255 = 255;
	int				t;

	dlog( "calling jpeg_idct_ifast_1x1_arm_v5\n");
	t = coef[0] * quant[0];;										
	t = 128 + (t>>5);
	CLIP(t,CONST_255); 
	output_buf[0][output_col] =t;	
}

//***bnie: need to optimize the following to arm v6
void jpeg_idct_ifast_4x4_arm_v6
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	JSAMPROW		out2 = output_buf[2] + output_col;
	JSAMPROW		out3 = output_buf[3] + output_col;
	TMP_TYPE		wk[32];
	int w12 = (w1<<16)|(w2&0x0000ffff);
	int w34 = (w3<<16)|(w4&0x0000ffff);

	dlog( "calling jpeg_idct_4x4_arm_v6\n");
	jpeg_idct_4x4_arm_v6(w12, w34, (int)wk, (int)quant, (int)coef, (int)out0, (int)out1, (int)out2, (int)out3);
}

void jpeg_idct_ifast_2x2_arm_v6
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	JSAMPROW		out0 = output_buf[0] + output_col;
	JSAMPROW		out1 = output_buf[1] + output_col;
	TMP_TYPE		wk[32];
	int w12 = (w1<<16)|(w2&0x0000ffff);
	int w34 = (w3<<16)|(w4&0x0000ffff);

	dlog( "calling jpeg_idct_ifast_2x2_arm_v6\n");
	jpeg_idct_2x2_arm_v6(w12, w34, (int)wk, (int)quant, (int)coef, (int)out0, (int)out1);
}

void jpeg_idct_ifast_1x1_arm_v6
(	
	j_decompress_ptr	cinfo, 
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	IFAST_MULT_TYPE	*quant = (IFAST_MULT_TYPE *) compptr->dct_table;
	int				CONST_255 = 255;
	int				t;

	dlog( "calling jpeg_idct_ifast_1x1_arm_v6\n");
	t = coef[0] * quant[0];
	t = 128 + (t>>5);
	CLIP(t,CONST_255); 
	output_buf[0][output_col] =t;	
}

void jpeg_idct_ifast_4x4_arm_v7
(																				
	j_decompress_ptr	cinfo,													
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	dlog( "calling jpeg_idct_ifast_4x4_arm_v7\n");
	jpeg_idct_4x4_arm_v7(compptr->dct_table, coef, output_buf, output_col);
}

void jpeg_idct_ifast_2x2_arm_v7
(																				
	j_decompress_ptr	cinfo,													
	jpeg_component_info *compptr,
	JCOEFPTR			coef,
	JSAMPARRAY			output_buf, 
	JDIMENSION			output_col
)
{
	dlog( "calling jpeg_idct_ifast_2x2_arm_v7\n");
	jpeg_idct_2x2_arm_v7(compptr->dct_table, coef, output_buf, output_col);
}
#endif


void ( *jpeg_idct_ifast_1x1_universal ) (	j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef, JSAMPARRAY output_buf, JDIMENSION output_col )=NULL;
void ( *jpeg_idct_ifast_2x2_universal ) (	j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef, JSAMPARRAY output_buf, JDIMENSION output_col )=NULL;
void ( *jpeg_idct_ifast_4x4_universal ) (	j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef, JSAMPARRAY output_buf, JDIMENSION output_col )=NULL;
void ( *jpeg_idct_ifast_8x8_universal ) (	j_decompress_ptr cinfo, jpeg_component_info *compptr, JCOEFPTR coef, JSAMPARRAY output_buf, JDIMENSION output_col )=NULL;

void ( *ycc_rgb565_convert_universal) (j_decompress_ptr cinfo, JSAMPIMAGE yuv, JDIMENSION input_row, JSAMPARRAY rgb, int num_rows);

void ycc_rgb565_convert_arm_v4 (j_decompress_ptr cinfo, JSAMPIMAGE yuv, JDIMENSION input_row, JSAMPARRAY rgb, int num_rows);
void ycc_rgb565_convert_arm_v6 (j_decompress_ptr cinfo, JSAMPIMAGE yuv, JDIMENSION input_row, JSAMPARRAY rgb, int num_rows);
void ycc_rgb565_convert_arm_v7 (j_decompress_ptr cinfo, JSAMPIMAGE yuv, JDIMENSION input_row, JSAMPARRAY rgb, int num_rows);
void ycc_rgb565_convert_fast_c (j_decompress_ptr cinfo, JSAMPIMAGE yuv, JDIMENSION input_row, JSAMPARRAY rgb, int num_rows);
void ycc_rgb565_convert_slow_c (j_decompress_ptr cinfo, JSAMPIMAGE yuv, JDIMENSION input_row, JSAMPARRAY rgb, int num_rows);

int g_UseOpenMaxIDCT = 0;

int config_ifast_idct(int implementation )
{
	int imp_out = FSK_ARCH_C;
	
	g_UseOpenMaxIDCT = 0;
	
	if( implementation == FSK_ARCH_AUTO )
		implementation = FskHardwareGetARMCPU_All();
	
	//if( implementation >= FSK_ARCH_ARM_V6 )
	//	implementation = FSK_ARCH_ARM_V6;
	
	dlog( "into config_ifast_idct, implementation: %d\n", implementation );
	
	switch( implementation )
	{
#ifdef KINOMA_YUV2RGB565_IFAST_ARM_SUPPORTED
		case FSK_ARCH_C:
		case FSK_ARCH_ARM_V4:
		case FSK_ARCH_ARM_V5:
		case FSK_ARCH_XSCALE:
			ycc_rgb565_convert_universal = ycc_rgb565_convert_arm_v4;
			imp_out = FSK_ARCH_ARM_V5;
			break;
		case FSK_ARCH_ARM_V6:
			ycc_rgb565_convert_universal = ycc_rgb565_convert_arm_v6;
			imp_out = FSK_ARCH_ARM_V6;
			break;
		case FSK_ARCH_ARM_V7:
			if(cap_neon_ycc_rgb()) {
				ycc_rgb565_convert_universal = ycc_rgb565_convert_arm_v7;
			} else
			 {
				ycc_rgb565_convert_universal = ycc_rgb565_convert_arm_v6;
			}
			imp_out = FSK_ARCH_ARM_V7;
			break;
#elif defined( KINOMA_YUV2RGB565_IFAST_C_SUPPORTED )
		case FSK_ARCH_C:
		default:
			ycc_rgb565_convert_universal = ycc_rgb565_convert_fast_c;
			imp_out = FSK_ARCH_C;
			break;
#else
		case FSK_ARCH_C:
		default:
			ycc_rgb565_convert_universal = ycc_rgb565_convert_slow_c;
			imp_out = FSK_ARCH_C;
			break;
#endif
	}


	switch( implementation )
	{
#ifdef IDCT_IFAST_8x8_ARM_V6_SUPPORTED
		case FSK_ARCH_ARM_V5:
		case FSK_ARCH_XSCALE:
			jpeg_idct_ifast_8x8_universal = jpeg_idct_ifast_8x8_arm_v5;
			imp_out = FSK_ARCH_ARM_V5;
		  break;

		case FSK_ARCH_ARM_V6:
#ifdef SUPPORT_OPENMAX
			jpeg_idct_ifast_8x8_universal = jpeg_idct_ifast_8x8_openmax_arm_v6;
			g_UseOpenMaxIDCT = 1;
#else
			jpeg_idct_ifast_8x8_universal = jpeg_idct_ifast_8x8_arm_v5;
#endif
			imp_out = FSK_ARCH_ARM_V6;
		  break;
		case FSK_ARCH_ARM_V7:
			jpeg_idct_ifast_8x8_universal = jpeg_idct_ifast_8x8_arm_v7;
			imp_out = FSK_ARCH_ARM_V7;
			break;
#endif
#ifdef IDCT_IFAST_8x8_C_SUPPORTED 
		case FSK_ARCH_C:
		default:
			jpeg_idct_ifast_8x8_universal = jpeg_idct_ifast_8x8_c;
			imp_out = FSK_ARCH_C;
			break;
#endif
	}

	switch( implementation )
	{
#ifdef IDCT_IFAST_SCALING_ARM_V6_SUPPORTED
		case FSK_ARCH_ARM_V5:
		case FSK_ARCH_XSCALE:
			jpeg_idct_ifast_1x1_universal = jpeg_idct_ifast_1x1_arm_v5;
			jpeg_idct_ifast_2x2_universal = jpeg_idct_ifast_2x2_arm_v5;
			jpeg_idct_ifast_4x4_universal = jpeg_idct_ifast_4x4_arm_v5;
			imp_out = FSK_ARCH_ARM_V5;
			break;

		case FSK_ARCH_ARM_V6:
			jpeg_idct_ifast_1x1_universal = jpeg_idct_ifast_1x1_arm_v6;
			jpeg_idct_ifast_2x2_universal = jpeg_idct_ifast_2x2_arm_v6;
			jpeg_idct_ifast_4x4_universal = jpeg_idct_ifast_4x4_arm_v6;
			imp_out = FSK_ARCH_ARM_V6;
			break;
		case FSK_ARCH_ARM_V7:
			jpeg_idct_ifast_1x1_universal = jpeg_idct_ifast_1x1_arm_v6;
			jpeg_idct_ifast_2x2_universal = jpeg_idct_ifast_2x2_arm_v7;
			jpeg_idct_ifast_4x4_universal = jpeg_idct_ifast_4x4_arm_v7;
			imp_out = FSK_ARCH_ARM_V7;
			break;
#endif

#ifdef IDCT_IFAST_SCALING_C_SUPPORTED 
		case FSK_ARCH_C:
		default:
			jpeg_idct_ifast_1x1_universal = jpeg_idct_ifast_1x1_c;
			jpeg_idct_ifast_2x2_universal = jpeg_idct_ifast_2x2_c;
			jpeg_idct_ifast_4x4_universal = jpeg_idct_ifast_4x4_c;
			imp_out = FSK_ARCH_C;
			break;
#endif
	}


//#ifdef SUPPORT_OPENMAX
//#ifdef IDCT_IFAST_8x8_C_SUPPORTED 
//***bnie: for win32 debug only, very very slow...
//			jpeg_idct_ifast_8x8_universal = jpeg_idct_ifast_8x8_openmax_c;
//			imp_out = FSK_ARCH_C;
//#endif
//#endif

	return imp_out;
}
