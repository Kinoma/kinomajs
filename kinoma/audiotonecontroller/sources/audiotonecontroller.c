/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "audiotonecontroller.h"

#define ROUND11		(1 << 10)
#define ROUND13		(1 << 12)
#define ROUND17		(1 << 16)

#undef DETECT_OVERFLOW
#ifdef DETECT_OVERFLOW
void DetectOverFlow( value, hi, lo )
{
	if( value > hi || value < lo )
	{
		printf( "overflow detected!\n");
	}
}
#else 
#define DetectOverFlow( value, hi, lo )
#endif
#define RANGE_CLIP( value, hi, lo )	{ DetectOverFlow( value, hi, lo ); value = value > hi ? hi : (value < lo ? lo : value ); }
//#define RANGE_CLIP( value, hi, lo )	{ value = value > hi ? hi : (value < lo ? lo : value ); }
#define APPLY_FILTER( f, s )	 ( f[0]*s[0] + f[1]*s[1] + f[2]*s[2] + f[3]*s[3] + f[4]*s[4] )

typedef struct 
{
	short		coef[10];	//f_lo[5], f_hi[5];//filter
	short		samples[20];//l_lo[5], l_hi[5], r_lo[5], r_hi[5];
}ToneControl;

short filter_coeff_44100[] =
{
	//44.1k: -32 ~ 32, lo/hi: 250/1000
	0x0fa3, 0xe175, 0x0eee, 0x1e85, 0xf169, 0x0435, 0xf85d, 0x037f, 0x1e83, 0xf16b,
	0x0fab, 0xe165, 0x0ef5, 0x1e95, 0xf15b, 0x0494, 0xf7b1, 0x03ce, 0x1e72, 0xf17b,
	0x0fb2, 0xe157, 0x0efc, 0x1ea4, 0xf14d, 0x04fb, 0xf6f6, 0x0424, 0x1e61, 0xf18a,
	0x0fb9, 0xe149, 0x0f03, 0x1eb3, 0xf13f, 0x056b, 0xf62b, 0x0481, 0x1e4e, 0xf19b,
	0x0fc0, 0xe13b, 0x0f0a, 0x1ec1, 0xf132, 0x05e5, 0xf54e, 0x04e6, 0x1e3b, 0xf1ac,
	0x0fc7, 0xe12e, 0x0f10, 0x1ece, 0xf126, 0x0669, 0xf45f, 0x0553, 0x1e27, 0xf1be,
	0x0fcd, 0xe122, 0x0f16, 0x1edb, 0xf11a, 0x06f8, 0xf35a, 0x05cb, 0x1e13, 0xf1d0,
	0x0fd3, 0xe116, 0x0f1c, 0x1ee8, 0xf10e, 0x0794, 0xf23f, 0x064c, 0x1dfd, 0xf1e4,
	0x0fd9, 0xe10b, 0x0f22, 0x1ef3, 0xf103, 0x083d, 0xf10c, 0x06d9, 0x1de7, 0xf1f7,
	0x0fdf, 0xe100, 0x0f27, 0x1eff, 0xf0f9, 0x08f5, 0xefbf, 0x0771, 0x1dcf, 0xf20c,
	0x0fe4, 0xe0f5, 0x0f2c, 0x1f09, 0xf0ee, 0x09bc, 0xee55, 0x0817, 0x1db6, 0xf222,
	0x0fe9, 0xe0eb, 0x0f31, 0x1f14, 0xf0e5, 0x0a94, 0xeccd, 0x08ca, 0x1d9d, 0xf238,
	0x0fee, 0xe0e1, 0x0f36, 0x1f1e, 0xf0db, 0x0b7f, 0xeb23, 0x098d, 0x1d82, 0xf24f,
	0x0ff3, 0xe0d8, 0x0f3a, 0x1f27, 0xf0d2, 0x0c7d, 0xe955, 0x0a61, 0x1d66, 0xf267,
	0x0ff7, 0xe0cf, 0x0f3f, 0x1f30, 0xf0ca, 0x0d91, 0xe761, 0x0b46, 0x1d49, 0xf280,
	0x0ffc, 0xe0c7, 0x0f43, 0x1f39, 0xf0c1, 0x0ebc, 0xe542, 0x0c3e, 0x1d2a, 0xf29a,
	0x1000, 0xe0bf, 0x0f47, 0x1f41, 0xf0b9, 0x1000, 0xe2f5, 0x0d4c, 0x1d0b, 0xf2b4,
	0x1004, 0xe0bf, 0x0f43, 0x1f41, 0xf0b9, 0x1160, 0xe054, 0x0e8d, 0x1d0b, 0xf2b4,
	0x1009, 0xe0bf, 0x0f3f, 0x1f41, 0xf0b9, 0x12df, 0xdd76, 0x0fec, 0x1d0b, 0xf2b4,
	0x100d, 0xe0bf, 0x0f3a, 0x1f41, 0xf0b9, 0x1480, 0xda55, 0x116c, 0x1d0b, 0xf2b4,
	0x1012, 0xe0bf, 0x0f36, 0x1f41, 0xf0b9, 0x1646, 0xd6ed, 0x130f, 0x1d0b, 0xf2b4,
	0x1017, 0xe0c0, 0x0f31, 0x1f41, 0xf0b9, 0x1833, 0xd335, 0x14d8, 0x1d0b, 0xf2b4,
	0x101c, 0xe0c0, 0x0f2c, 0x1f41, 0xf0b9, 0x1a4d, 0xcf28, 0x16cc, 0x1d0b, 0xf2b4,
	0x1022, 0xe0c0, 0x0f27, 0x1f41, 0xf0b9, 0x1c96, 0xcabe, 0x18ee, 0x1d0b, 0xf2b4,
	0x1027, 0xe0c1, 0x0f22, 0x1f41, 0xf0b9, 0x1f13, 0xc5ed, 0x1b41, 0x1d0b, 0xf2b4,
	0x102d, 0xe0c1, 0x0f1c, 0x1f41, 0xf0b9, 0x21c8, 0xc0ad, 0x1dcb, 0x1d0b, 0xf2b4,
	0x1033, 0xe0c1, 0x0f16, 0x1f41, 0xf0b9, 0x24bb, 0xbaf4, 0x2091, 0x1d0b, 0xf2b4,
	0x103a, 0xe0c2, 0x0f10, 0x1f41, 0xf0b9, 0x27f1, 0xb4b7, 0x2398, 0x1d0b, 0xf2b4,
	0x1041, 0xe0c2, 0x0f0a, 0x1f41, 0xf0b9, 0x2b71, 0xadea, 0x26e6, 0x1d0b, 0xf2b4,
	0x1048, 0xe0c3, 0x0f03, 0x1f41, 0xf0b9, 0x2f3f, 0xa680, 0x2a82, 0x1d0b, 0xf2b4,
	0x104f, 0xe0c4, 0x0efc, 0x1f41, 0xf0b9, 0x3365, 0x9e6b, 0x2e72, 0x1d0b, 0xf2b4,
	0x1057, 0xe0c4, 0x0ef5, 0x1f41, 0xf0b9, 0x37e9, 0x959b, 0x32be, 0x1d0b, 0xf2b4,
	0x105f, 0xe0c5, 0x0eee, 0x1f41, 0xf0b9, 0x3cd3, 0x8c00, 0x376e, 0x1d0b, 0xf2b4
};

short filter_coeff_48000[] =
{
	//48k: -32 ~ 32, lo/hi: 250/1000
	0x0f9b, 0xe195, 0x0ed6, 0x1e64, 0xf188, 0x043a, 0xf867, 0x0374, 0x1e61, 0xf18a,
	0x0fa3, 0xe184, 0x0ede, 0x1e75, 0xf178, 0x0499, 0xf7bc, 0x03c2, 0x1e4f, 0xf19a,
	0x0fab, 0xe174, 0x0ee6, 0x1e86, 0xf169, 0x0500, 0xf702, 0x0416, 0x1e3c, 0xf1ab,
	0x0fb3, 0xe165, 0x0eee, 0x1e96, 0xf15a, 0x0570, 0xf639, 0x0472, 0x1e28, 0xf1bd,
	0x0fbb, 0xe157, 0x0ef5, 0x1ea5, 0xf14c, 0x05ea, 0xf55e, 0x04d5, 0x1e13, 0xf1d0,
	0x0fc2, 0xe149, 0x0efc, 0x1eb3, 0xf13f, 0x066e, 0xf470, 0x0541, 0x1dfe, 0xf1e3,
	0x0fc9, 0xe13b, 0x0f02, 0x1ec1, 0xf132, 0x06fd, 0xf36e, 0x05b7, 0x1de7, 0xf1f7,
	0x0fcf, 0xe12e, 0x0f08, 0x1ecf, 0xf125, 0x0799, 0xf256, 0x0636, 0x1dd0, 0xf20b,
	0x0fd6, 0xe122, 0x0f0f, 0x1edc, 0xf119, 0x0842, 0xf126, 0x06c0, 0x1db7, 0xf221,
	0x0fdc, 0xe116, 0x0f14, 0x1ee8, 0xf10e, 0x08f9, 0xefdc, 0x0756, 0x1d9d, 0xf237,
	0x0fe2, 0xe10b, 0x0f1a, 0x1ef4, 0xf103, 0x09c0, 0xee76, 0x07f9, 0x1d83, 0xf24e,
	0x0fe7, 0xe100, 0x0f1f, 0x1eff, 0xf0f8, 0x0a98, 0xecf1, 0x08a9, 0x1d67, 0xf266,
	0x0fed, 0xe0f5, 0x0f24, 0x1f0a, 0xf0ee, 0x0b82, 0xeb4c, 0x0969, 0x1d4a, 0xf27f,
	0x0ff2, 0xe0eb, 0x0f29, 0x1f14, 0xf0e4, 0x0c80, 0xe984, 0x0a38, 0x1d2b, 0xf299,
	0x0ff7, 0xe0e1, 0x0f2e, 0x1f1e, 0xf0db, 0x0d93, 0xe795, 0x0b19, 0x1d0c, 0xf2b3,
	0x0ffb, 0xe0d8, 0x0f32, 0x1f28, 0xf0d2, 0x0ebd, 0xe57d, 0x0c0d, 0x1ceb, 0xf2cf,
	0x1000, 0xe0cf, 0x0f37, 0x1f31, 0xf0c9, 0x1000, 0xe338, 0x0d15, 0x1cc8, 0xf2eb,
	0x1005, 0xe0d0, 0x0f32, 0x1f31, 0xf0c9, 0x115e, 0xe09c, 0x0e52, 0x1cc8, 0xf2eb,
	0x1009, 0xe0d0, 0x0f2e, 0x1f31, 0xf0c9, 0x12dc, 0xddc4, 0x0fad, 0x1cc8, 0xf2eb,
	0x100e, 0xe0d0, 0x0f29, 0x1f31, 0xf0c9, 0x147b, 0xdaaa, 0x1128, 0x1cc8, 0xf2eb,
	0x1013, 0xe0d0, 0x0f24, 0x1f31, 0xf0c9, 0x163e, 0xd748, 0x12c6, 0x1cc8, 0xf2eb,
	0x1019, 0xe0d1, 0x0f1f, 0x1f31, 0xf0c9, 0x182a, 0xd399, 0x148a, 0x1cc8, 0xf2eb,
	0x101f, 0xe0d1, 0x0f1a, 0x1f31, 0xf0c9, 0x1a40, 0xcf94, 0x1678, 0x1cc8, 0xf2eb,
	0x1025, 0xe0d1, 0x0f14, 0x1f31, 0xf0c9, 0x1c86, 0xcb33, 0x1893, 0x1cc8, 0xf2eb,
	0x102b, 0xe0d2, 0x0f0f, 0x1f31, 0xf0c9, 0x1f00, 0xc66c, 0x1ae0, 0x1cc8, 0xf2eb,
	0x1031, 0xe0d2, 0x0f08, 0x1f31, 0xf0c9, 0x21b2, 0xc137, 0x1d63, 0x1cc8, 0xf2eb,
	0x1038, 0xe0d3, 0x0f02, 0x1f31, 0xf0c9, 0x24a1, 0xbb8a, 0x2021, 0x1cc8, 0xf2eb,
	0x103f, 0xe0d3, 0x0efc, 0x1f31, 0xf0c9, 0x27d2, 0xb55a, 0x2320, 0x1cc8, 0xf2eb,
	0x1047, 0xe0d4, 0x0ef5, 0x1f31, 0xf0c9, 0x2b4c, 0xae9b, 0x2665, 0x1cc8, 0xf2eb,
	0x104e, 0xe0d5, 0x0eee, 0x1f31, 0xf0c9, 0x2f15, 0xa741, 0x29f7, 0x1cc8, 0xf2eb,
	0x1057, 0xe0d5, 0x0ee6, 0x1f31, 0xf0c9, 0x3334, 0x9f3c, 0x2ddc, 0x1cc8, 0xf2eb,
	0x105f, 0xe0d6, 0x0ede, 0x1f31, 0xf0c9, 0x37b1, 0x967f, 0x321d, 0x1cc8, 0xf2eb,
	0x1068, 0xe0d7, 0x0ed6, 0x1f31, 0xf0c9, 0x3c94, 0x8cf8, 0x36c1, 0x1cc8, 0xf2eb
};

#if 1

/*
little endian packing

x0_b,	coef[0]
x0_t,	coef[1]
x1_b,	coef[2]
x1_t,	coef[3]
x2_b,	coef[4]				
x2_t,	coef[5]
x3_b,	coef[6]
x3_t,	coef[7]
x4_b,	coef[8]
x4_t,	coef[9]
		
x5_b,	lo[0]
x5_t,	lo[1]
x6_b,	lo[2]
x6_t,	lo[3]
x7_b,	lo[4]					
x7_t,	hi[0]
x8_b,	hi[1]
x8_t,	hi[2]
x9_b	hi[3]
x9_t,	hi[4]
*/


#define	process_5(																					\
	src,																							\
	dst,																							\
																									\
	pitch,																							\
																									\
	x0_b,																							\
	x0_t,																							\
	x1_b,																							\
	x1_t,																							\
	x2_b,																							\
																									\
	x2_t,																							\
	x3_b,																							\
	x3_t,																							\
	x4_b,																							\
	x4_t,																							\
																									\
	x5_b,																							\
	x5_t,																							\
	x6_b,																							\
	x6_t,																							\
	x7_b,																							\
																									\
	x7_t,																							\
	x8_b,																							\
	x8_t,																							\
	x9_b,																							\
	x9_t																							\
)																									\
{																									\
	int s, out, out_lo, out_hi;																		\
																									\
	/*0*/																							\
	src += pitch;																					\
	s = (*src) >> 2;																				\
	x5_b = s;																						\
	out_lo = x0_b*x5_b + x0_t* x5_t + x1_b*x6_b + x1_t*x6_t + x2_b*x7_b;						\
																									\
	out = (out_lo + ROUND13) >> 12;																	\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x6_b = (short)(out);																			\
																									\
	x7_t = s;																						\
	out_hi = x2_t*x7_t + x3_b*x8_b + x3_t*x8_t + x4_b*x9_b + x4_t*x9_t;						\
																									\
	out = (out_hi  + ROUND13) >> 12;																\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x8_t = (short)(out);																			\
																									\
	out = (out_lo + out_hi + ROUND11) >> 11;														\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	dst += pitch;																					\
	*(dst) = (short)(out);																			\
																									\
	/*1*/																							\
	src += pitch;																					\
	s = (*src) >> 2;																				\
	x7_b = s;																						\
	out_lo = x0_b*x7_b + x0_t*x5_b + x1_b* x5_t + x1_t*x6_b + x2_b*x6_t;						\
																									\
	out = (out_lo + ROUND13) >> 12;																	\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	 x5_t = (short)(out);																			\
																									\
	x9_t = s;																						\
	out_hi = x2_t*x9_t + x3_b*x7_t + x3_t*x8_b + x4_b*x8_t + x4_t*x9_b;						\
																									\
	out = (out_hi  + ROUND13) >> 12;																\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x8_b = (short)(out);																			\
																									\
	out = (out_lo + out_hi + ROUND11) >> 11;														\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	dst += pitch;																					\
	*(dst) = (short)(out);																			\
																									\
	/*2*/																							\
	src += pitch;																					\
	s = (*src) >> 2;																				\
	x6_t = s;																						\
	out_lo = x0_b*x6_t + x0_t*x7_b + x1_b*x5_b + x1_t* x5_t + x2_b*x6_b;						\
																									\
	out = (out_lo + ROUND13) >> 12;																	\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x5_b = (short)(out);																			\
																									\
	x9_b = s;																						\
	out_hi = x2_t*x9_b + x3_b*x9_t + x3_t*x7_t + x4_b*x8_b + x4_t*x8_t;						\
																									\
	out = (out_hi  + ROUND13) >> 12;																\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x7_t = (short)(out);																			\
																									\
	out = (out_lo + out_hi + ROUND11) >> 11;														\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	dst += pitch;																					\
	*(dst) = (short)(out);																			\
																									\
	/*3*/																							\
	src += pitch;																					\
	s = (*src) >> 2;																				\
	x6_b = s;																						\
	out_lo = x0_b*x6_b + x0_t*x6_t + x1_b*x7_b + x1_t*x5_b + x2_b* x5_t;						\
																									\
	out = (out_lo + ROUND13) >> 12;																	\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x7_b = (short)(out);																			\
																									\
	x8_t = s;																						\
	out_hi = x2_t*x8_t + x3_b*x9_b + x3_t*x9_t + x4_b*x7_t + x4_t*x8_b;						\
																									\
	out = (out_hi  + ROUND13) >> 12;																\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x9_t = (short)(out);																			\
																									\
	out = (out_lo + out_hi + ROUND11) >> 11;														\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	dst += pitch;																					\
	*(dst) = (short)(out);																			\
																									\
	/*4*/																							\
	src += pitch;																					\
	s = (*src) >> 2;																				\
	 x5_t = s;																						\
	out_lo = x0_b* x5_t + x0_t*x6_b + x1_b*x6_t + x1_t*x7_b + x2_b*x5_b;						\
																									\
	out = (out_lo + ROUND13) >> 12;																	\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x6_t = (short)(out);																			\
																									\
	x8_b = s;																						\
	out_hi = x2_t*x8_b + x3_b*x8_t + x3_t*x9_b + x4_b*x9_t + x4_t*x7_t;						\
																									\
	out = (out_hi  + ROUND13) >> 12;																\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	x9_b = (short)(out);																			\
																									\
	out = (out_lo + out_hi + ROUND11) >> 11;														\
	RANGE_CLIP( out, 32767, -32767 );																\
																									\
	dst += pitch;																					\
	*(dst) = (short)(out);																			\
																									\
}

void ToneControlMono(ToneControl *t, long sampleCount, short *src0, short *dst0)
{
	long	i;
	long	out_lo, out_hi, out;
	short	s;
	short   *l = &t->samples[0];
	short	*c = &t->coef[0]; 
	short	cnt5 = (short)(sampleCount/5);
	short	cnt4 = (short)(sampleCount - cnt5 * 5);
	short	*src = src0 - 1;
	short	*dst = dst0 - 1;

	for( i = 0; i < cnt5; i++ )
	{
		process_5
		(			
			src,	
			dst,	
			1,
			c[0], c[1],	c[2], c[3],	c[4], c[5],	c[6], c[7],	c[8], c[9],
			l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7], l[8], l[9] 
		);
	}

	for (i = 0; i < cnt4; i++ )
	{
		s = (*(++src)) >> 2;
		l[0] = s;
		out_lo = APPLY_FILTER( c, l );

		out = (out_lo + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );

		l[4] = l[3];
		l[3] = (short)(out);	
		l[2] =  l[1];
		l[1] = l[0];

		l[5] = s;
		out_hi = APPLY_FILTER( (c+5), (l+5) );
 
		out = (out_hi  + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );
		
		l[9] = l[8];
		l[8] = (short)(out);							
		l[7] = l[6];
		l[6] = l[5];			
		
		out = (out_lo + out_hi + ROUND11) >> 11;
		RANGE_CLIP( out, 32767, -32767 );
		
		*(++dst) = (short)(out);
	}
}

void ToneControlStereo(ToneControl *t, long sampleCount, short *source, short *dest)
{
	long	i;
	long	out_lo, out_hi, out;
	short	s;
	short	*l	  = &t->samples[0];
	short	*r	  = &t->samples[10];
	short	*f    = &t->coef[0]; 
	short	cnt5  = (short)(sampleCount/5);
	short	cnt4  = (short)(sampleCount - cnt5 * 5);
	short	*src  = source - 2;
	short	*dst  = dest   - 2;

	for( i = 0; i < cnt5; i++ )
	{
		process_5
		(			
			src,	
			dst,	
			2,
			f[0], f[1], f[2], f[3],	f[4], f[5],	f[6], f[7],	f[8], f[9],
			l[0], l[1],	l[2], l[3], l[4], l[5], l[6], l[7], l[8], l[9] 
		);
		
		src -= 9;
		dst -= 9;

		process_5
		(			
			src,	
			dst,	
			2,
			f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9],
			r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9] 
		);

		src -= 1;
		dst -= 1;
	}

	src += 1;
	dst += 1;
	for (i = 0; i < cnt4; i++ )
	{
		s = (*(++src)) >> 2;
		l[0] = s;
		out_lo = APPLY_FILTER( f, l );

		out = (out_lo + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );

		l[4] = l[3];
		l[3] = (short)(out);	
		l[2] = l[1];
		l[1] = l[0];

		l[5] = s;
		out_hi = APPLY_FILTER( (f+5), (l+5) );
 
		out = (out_hi  + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );
		
		l[9] = l[8];
		l[8] = (short)(out);							
		l[7] = l[6];
		l[6] = l[5];			
		
		out = (out_lo + out_hi + ROUND11) >> 11;
		RANGE_CLIP( out, 32767, -32767 );
		
		*(++dst) = (short)(out);
		
		//===//
		s = (*(++src)) >> 2;
		r[0] = s;
		out_lo = APPLY_FILTER( f, r );

		out = (out_lo + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );

		r[4] = r[3];
		r[3] = (short)(out);	
		r[2] = r[1];
		r[1] = r[0];

		r[5] = s;
		out_hi = APPLY_FILTER( (f+5), (r+5) );
 
		out = (out_hi  + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );
		
		r[9] = r[8];
		r[8] = (short)(out);							
		r[7] = r[6];
		r[6] = r[5];			
		
		out = (out_lo + out_hi + ROUND11) >> 11;
		RANGE_CLIP( out, 32767, -32767 );
		
		*(++dst) = (short)(out);
	}
}

#else
void ToneControlMono(ToneControl *t, long sampleCount, short *source, short *dest)
{
	long	i;
	long	out_lo, out_hi, out;
	short	src;
	short   *l_lo = &t->samples[0];
	short	*l_hi = &t->samples[5];
	short	*fl = &t->coef[0]; 
	short	*fh = &t->coef[5];

	for (i = 0; i < sampleCount; i++)
	{
		src = source[i] >> 2;
		l_lo[0] = src;
		out_lo = APPLY_FILTER( fl, l_lo );

		out = (out_lo + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );

		l_lo[4] = l_lo[3];
		l_lo[3] = (short)(out);	
		l_lo[2] = l_lo[1];
		l_lo[1] = l_lo[0];

		l_hi[0] = src;
		out_hi = APPLY_FILTER( fh, l_hi );
 
		out = (out_hi  + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );
		
		l_hi[4] = l_hi[3];
		l_hi[3] = (short)(out);							
		l_hi[2] = l_hi[1];
		l_hi[1] = l_hi[0];			
		
		out = (out_lo + out_hi + ROUND11) >> 11;
		RANGE_CLIP( out, 32767, -32767 );
		
		dest[i] = (short)(out);		
	}
}
void ToneControlStereo(ToneControl *t, long sampleCount, short *source, short *dest)
{
	long		i;
	long		out_lo, out_hi, out;
	short		src;
	short		*l_lo = &t->samples[0];
	short		*l_hi = &t->samples[5];
	short		*r_lo = &t->samples[10];
	short		*r_hi = &t->samples[15];
	short		*f_lo = &t->coef[0]; 
	short		*f_hi = &t->coef[5];
		
	for (i = 0; i < sampleCount; i++)
	{
		src = (*source++) >> 2;
		l_lo[0] = src;
		out_lo = APPLY_FILTER( f_lo, l_lo );
		out = (out_lo + ROUND13) >> 12;		
		RANGE_CLIP( out, 32767, -32767 );

		l_lo[4] = l_lo[3];
		l_lo[3] = (short)(out);		
		l_lo[2] = l_lo[1];
		l_lo[1] = l_lo[0];								
	
		l_hi[0] = src;
		out_hi = APPLY_FILTER( f_hi, l_hi );
		out = (out_hi  + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );

		l_hi[4] = l_hi[3];
		l_hi[3] = (short)(out);
		l_hi[2] = l_hi[1];
		l_hi[1] = l_hi[0];
		
		out = (out_lo + out_hi + ROUND11) >> 11;
		RANGE_CLIP( out, 32767, -32767 );
		*dest++ = (short)(out);

		src = (*source++) >> 2;
		r_lo[0] = src;
		out_lo = APPLY_FILTER( f_lo, r_lo );
		out = (out_lo + ROUND13) >> 12;	
		RANGE_CLIP( out, 32767, -32767 );

		r_lo[4] = r_lo[3];
		r_lo[3] = (short)(out);			
		r_lo[2] = r_lo[1];
		r_lo[1] = r_lo[0];							

		r_hi[0] = src;
		out_hi = APPLY_FILTER( f_hi, r_hi );
		out = (out_hi  + ROUND13) >> 12;
		RANGE_CLIP( out, 32767, -32767 );

		r_hi[4] = r_hi[3];
		r_hi[3] = (short)(out);	
		r_hi[2] = r_hi[1];
		r_hi[1] = r_hi[0];

		out = (out_lo + out_hi +  ROUND11) >> 11;
		RANGE_CLIP( out, 32767, -32767 );
		*dest++ = (short)(out);
	}
}


#endif

int GetFilter(int lo, int hi, int sampleRate, ToneControl *t)
{
	int lo_idx;
	int hi_idx;
	short	*f;
	short	*tab;
	short   *c;
		
	if( lo < -16 ) lo = -16;
	if( lo >  16 ) lo =  16;
	if( hi < -16 ) hi = -16;
	if( hi >  16 ) hi =  16;

	lo_idx = (lo + 16)*10;
	hi_idx = (hi + 16)*10 + 5;

	if( sampleRate == 44100 )
		tab = &filter_coeff_44100[0];
	else if(  sampleRate == 48000 )
		tab = &filter_coeff_48000[0];
	else
		return -1;

	f = &t->coef[0];
	c = &tab[lo_idx];
	FskMemCopy( f, c, sizeof(short) * 5 );

	f = &t->coef[5];
	c = &tab[hi_idx];
	FskMemCopy( f, c, sizeof(short) * 5 );

	return 0;
}


typedef struct 
{
	int			bass;
	int			treble;
	ToneControl	tc;
}ToneControllerStateRecord, *ToneControllerState;

Boolean	tonecontrollerCanHandle(const char *filterType)
{
	return (0 == FskStrCompareCaseInsensitive("tone controller", filterType));
}

FskErr tonecontrollerNew(FskAudioFilter filter, void **filterState)
{
	FskErr				err;
	ToneControllerState state;

	err = FskMemPtrNewClear(sizeof(ToneControllerStateRecord), &state);
	if( err ) goto bail;

bail:
	if( err ) 
	{
		tonecontrollerDispose(filter, state);
		state = NULL;
	}

	*filterState = state;

	return err;
}

FskErr tonecontrollerDispose(FskAudioFilter filter, void *stateIn)
{
	ToneControllerState state = stateIn;

	FskMemPtrDispose(state);

	return kFskErrNone;
}

FskErr tonecontrollerGetMaximumBufferSize(FskAudioFilter filter, void *stateIn, UInt32 sampleCount, UInt32 *bufferSize)
{
	int				outputSampleCount;
	
	outputSampleCount = sampleCount;
	*bufferSize = outputSampleCount * 2 * filter->inputChannelCount;

	return kFskErrNone;
}

FskErr tonecontrollerStart(FskAudioFilter filter, void *stateIn)
{
	ToneControllerState		state = stateIn;
	FskErr					err = kFskErrNone;
	
#if TARGET_RT_LITTLE_ENDIAN
	if (kFskAudioFormatPCM16BitLittleEndian != filter->inputFormat)
		return kFskErrInvalidParameter;
#else
	if (kFskAudioFormatPCM16BitBigEndian != filter->inputFormat)
		return kFskErrInvalidParameter;
#endif
	
	err = GetFilter( state->bass, state->treble, filter->inputSampleRate, &state->tc );

	return err;
}

FskErr tonecontrollerStop(FskAudioFilter filter, void *stateIn)
{
	return kFskErrNone;
}

FskErr tonecontrollerProcessSamples(FskAudioFilter filter, void *stateIn, void *input, UInt32 inputSampleCount, void *output, UInt32 *outputSampleCount, UInt32 *outputSize)
{
	ToneControllerState	state		= stateIn;
	FskErr				err			= kFskErrNone;

	// flush, nothing to do
	if ((NULL == input) || (0 == inputSampleCount))
		 goto bail;

	if( filter->inputChannelCount == 1 )
		ToneControlMono(&state->tc, inputSampleCount, (short *)input, (short *)output);
	else
		ToneControlStereo(&state->tc, inputSampleCount, (short *)input, (short *)output);
	
	*outputSampleCount	= inputSampleCount;
	*outputSize			= inputSampleCount * 2 * filter->inputChannelCount;

bail:
	return err;
}

void GetNextWord( char *s, long *idx, char *w )
{
	long i = *idx;
	long j = 0;
	
	while( s[i] != ';' && s[i] != ',' && s[i] != 0 )
	{
		w[j] = s[i];
		i++;
		j++;
	}
	
	w[j] = 0;
	if( s[i] == 0 )
		*idx = i;
	else
		*idx = i+1;
}

void GetKeyAndValue( char *w, char *k, char *v )
{
	long i = 0, j = 0;

	while( w[i] != 0 && w[i] != '=' && w[i] != ':' )
	{
		k[j] = w[i];
		i++;
		j++;
	}
	k[j] = 0;
	
	j = 0;
	while( w[i] != 0  )
	{
		if( w[i] != '=' && w[i] != ':' )
		{
			v[j] = w[i];
			j++;
		}
		
		i++;
	}
	v[j] = 0;
}


FskErr tonecontrollerSetSettings(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	ToneControllerState	state	= (ToneControllerState)stateIn;
	char			*s		= property->value.str;
	long			i		= 0;
	FskErr			err		= kFskErrNone;


	state->bass = 0;
	state->treble = 0;

	if (NULL == s)
		return kFskErrNone;

	while(1)
	{
		char w[64];
		char k[32], v[32];

		GetNextWord( s, &i, w );
		if( w[0] == 0 )
			break;
		
		GetKeyAndValue( w, k, v );
		
		if( FskStrCompare( k, "bass" ) == 0 )
		{
			state->bass = atoi( v );
		}
		else if( FskStrCompare( k, "treble" ) == 0 )
		{
			state->treble = atoi( v );
		}
	}

	return err;
}
