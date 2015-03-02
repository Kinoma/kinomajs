/*
 * jccolor.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains input colorspace conversion routines.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "FskArch.h"


/* Private subobject */

typedef struct {
  struct jpeg_color_converter pub; /* public fields */

  /* Private state for RGB->YCC conversion */
  INT32 * rgb_ycc_tab;		/* => table for RGB to YCbCr conversion */
} my_color_converter;

typedef my_color_converter * my_cconvert_ptr;


/**************** RGB -> YCbCr conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + CENTERJSAMPLE
 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + CENTERJSAMPLE
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 * Note: older versions of the IJG code used a zero offset of MAXJSAMPLE/2,
 * rather than CENTERJSAMPLE, for Cb and Cr.  This gave equal positive and
 * negative swings for Cb/Cr, but meant that grayscale values (Cb=Cr=0)
 * were not represented exactly.  Now we sacrifice exact representation of
 * maximum red and maximum blue in order to get exact grayscales.
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times R,G,B for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The CENTERJSAMPLE offsets and the rounding fudge-factor of 0.5 are included
 * in the tables to save adding them separately in the inner loop.
 */

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define CBCR_OFFSET	((INT32) CENTERJSAMPLE << SCALEBITS)
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))

/* We allocate one big table and divide it up into eight parts, instead of
 * doing eight alloc_small requests.  This lets us use a single table base
 * address, which can be held in a register in the inner loops on many
 * machines (more than can hold all eight addresses, anyway).
 */

#define R_Y_OFF		0			/* offset to R => Y section */
#define G_Y_OFF		(1*(MAXJSAMPLE+1))	/* offset to G => Y section */
#define B_Y_OFF		(2*(MAXJSAMPLE+1))	/* etc. */
#define R_CB_OFF	(3*(MAXJSAMPLE+1))
#define G_CB_OFF	(4*(MAXJSAMPLE+1))
#define B_CB_OFF	(5*(MAXJSAMPLE+1))
#define R_CR_OFF	B_CB_OFF		/* B=>Cb, R=>Cr are the same */
#define G_CR_OFF	(6*(MAXJSAMPLE+1))
#define B_CR_OFF	(7*(MAXJSAMPLE+1))
#define TABLE_SIZE	(8*(MAXJSAMPLE+1))


/*
 * Initialize for RGB->YCC colorspace conversion.
 */

METHODDEF(void)
rgb_ycc_start (j_compress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  INT32 * rgb_ycc_tab;
  INT32 i;

  /* Allocate and fill in the conversion tables. */
  cconvert->rgb_ycc_tab = rgb_ycc_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(TABLE_SIZE * SIZEOF(INT32)));

  for (i = 0; i <= MAXJSAMPLE; i++) {
    rgb_ycc_tab[i+R_Y_OFF] = FIX(0.29900) * i;
    rgb_ycc_tab[i+G_Y_OFF] = FIX(0.58700) * i;
    rgb_ycc_tab[i+B_Y_OFF] = FIX(0.11400) * i     + ONE_HALF;
    rgb_ycc_tab[i+R_CB_OFF] = (-FIX(0.16874)) * i;
    rgb_ycc_tab[i+G_CB_OFF] = (-FIX(0.33126)) * i;
    /* We use a rounding fudge-factor of 0.5-epsilon for Cb and Cr.
     * This ensures that the maximum output will round to MAXJSAMPLE
     * not MAXJSAMPLE+1, and thus that we don't have to range-limit.
     */
    rgb_ycc_tab[i+B_CB_OFF] = FIX(0.50000) * i    + CBCR_OFFSET + ONE_HALF-1;
/*  B=>Cb and R=>Cr tables are the same
    rgb_ycc_tab[i+R_CR_OFF] = FIX(0.50000) * i    + CBCR_OFFSET + ONE_HALF-1;
*/
    rgb_ycc_tab[i+G_CR_OFF] = (-FIX(0.41869)) * i;
    rgb_ycc_tab[i+B_CR_OFF] = (-FIX(0.08131)) * i;
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 *
 * Note that we change from the application's interleaved-pixel format
 * to our internal noninterleaved, one-plane-per-component format.
 * The input buffer is therefore three times as wide as the output buffer.
 *
 * A starting row offset is provided only for the output buffer.  The caller
 * can easily adjust the passed input_buf value to accommodate any row
 * offset required on that side.
 */

#if KINOMA_DCT_IFAST_SUPPORTED
LOCAL(void)
rgb24_to_yuv444_neon (unsigned char *rgb,
			unsigned char *y, unsigned char *u, unsigned char *v,
			  		      int n)
{
	const static unsigned short c[16] = {
	FIX(0.29900), FIX(0.58700), FIX(0.11400), /*  d0[0],  d0[1],  d0[2] */
	FIX(0.16874), FIX(0.33126), FIX(0.50000), /* -d0[3], -d1[0],  d1[1] */
	FIX(0.41869), FIX(0.08131),               /*  d1[1], -d1[2], -d1[3] */
	0x7FFF, 128, 0x7FFF, 128, 0x7FFF, 128, 0x7FFF, 128
	};
#if !(__APPLE__)
	asm volatile(".fpu neon\n");
#endif	
	asm volatile (
		/* r = d10, g = d11, b = d12 */
		".macro do_rgb_to_yuv\n"
		"   vmovl.u8   q2, d10\n" /* r = { d4, d5 } */
		"   vmovl.u8   q3, d11\n" /* g = { d6, d7 } */
		"   vmovl.u8   q4, d12\n" /* b = { d8, d9 } */
		
		"   vmull.u16   q10, d4, d0[0]\n"
		"   vmlal.u16   q10, d6, d0[1]\n"
		"   vmlal.u16   q10, d8, d0[2]\n"
		"   vmull.u16   q11, d5, d0[0]\n"
		"   vmlal.u16   q11, d7, d0[1]\n"
		"   vmlal.u16   q11, d9, d0[2]\n"
		
		"   vmov.u32    q12, q1\n"
		"   vmov.u32    q13, q1\n"
		"   vmlsl.u16   q12, d4, d0[3]\n"
		"   vmlsl.u16   q12, d6, d1[0]\n"
		"   vmlal.u16   q12, d8, d1[1]\n"
		"   vmlsl.u16   q13, d5, d0[3]\n"
		"   vmlsl.u16   q13, d7, d1[0]\n"
		"   vmlal.u16   q13, d9, d1[1]\n"
		
		"   vmov.u32    q14, q1\n"
		"   vmov.u32    q15, q1\n"
		"   vmlal.u16   q14, d4, d1[1]\n"
		"   vmlsl.u16   q14, d6, d1[2]\n"
		"   vmlsl.u16   q14, d8, d1[3]\n"
		"   vmlal.u16   q15, d5, d1[1]\n"
		"   vmlsl.u16   q15, d7, d1[2]\n"
		"   vmlsl.u16   q15, d9, d1[3]\n"

		"   vrshrn.u32  d20, q10, #16\n"
		"   vrshrn.u32  d21, q11, #16\n"
		"   vshrn.u32   d24, q12, #16\n"
		"   vshrn.u32   d25, q13, #16\n"
		"   vshrn.u32   d28, q14, #16\n"
		"   vshrn.u32   d29, q15, #16\n"

		"   vmovn.u16   d20, q10\n"      /* d20 = y */
		"   vmovn.u16   d21, q12\n"      /* d21 = u */
		"   vmovn.u16   d22, q14\n"      /* d22 = v */
		".endm\n"
		".macro do_load size\n"
			".if \\size == 8\n"
				"vld3.8 {d10, d11, d12}, [%[rgb]]!\n"
				"pld [%[rgb], #128]\n"
			".elseif \\size == 4\n"
				"vld3.8 {d10[0], d11[0], d12[0]}, [%[rgb]]!\n"
				"vld3.8 {d10[1], d11[1], d12[1]}, [%[rgb]]!\n"
				"vld3.8 {d10[2], d11[2], d12[2]}, [%[rgb]]!\n"
				"vld3.8 {d10[3], d11[3], d12[3]}, [%[rgb]]!\n"
			".elseif \\size == 2\n"
				"vld3.8 {d10[4], d11[4], d12[4]}, [%[rgb]]!\n"
				"vld3.8 {d10[5], d11[5], d12[5]}, [%[rgb]]!\n"
			".elseif \\size == 1\n"
				"vld3.8 {d10[6], d11[6], d12[6]}, [%[rgb]]!\n"
			".else\n"
				".error \"unsupported macroblock size\"\n"
			".endif\n"
		".endm\n"
		".macro do_store size\n"
			".if \\size == 8\n"
				"vst1.8  {d21}, [%[u]]!\n"
				"vst1.8  {d22}, [%[v]]!\n"
				"vst1.8  {d20}, [%[y]]!\n"
			".elseif \\size == 4\n"
				"vst1.8  {d21[0]}, [%[u]]!\n"
				"vst1.8  {d21[1]}, [%[u]]!\n"
				"vst1.8  {d21[2]}, [%[u]]!\n"
				"vst1.8  {d21[3]}, [%[u]]!\n"
				"vst1.8  {d22[0]}, [%[v]]!\n"
				"vst1.8  {d22[1]}, [%[v]]!\n"
				"vst1.8  {d22[2]}, [%[v]]!\n"
				"vst1.8  {d22[3]}, [%[v]]!\n"
				"vst1.8  {d20[0]}, [%[y]]!\n"
				"vst1.8  {d20[1]}, [%[y]]!\n"
				"vst1.8  {d20[2]}, [%[y]]!\n"
				"vst1.8  {d20[3]}, [%[y]]!\n"
			".elseif \\size == 2\n"
				"vst1.8  {d21[4]}, [%[u]]!\n"
				"vst1.8  {d21[5]}, [%[u]]!\n"
				"vst1.8  {d22[4]}, [%[v]]!\n"
				"vst1.8  {d22[5]}, [%[v]]!\n"
				"vst1.8  {d20[4]}, [%[y]]!\n"
				"vst1.8  {d20[5]}, [%[y]]!\n"
			".elseif \\size == 1\n"
				"vst1.8  {d21[6]}, [%[u]]!\n"
				"vst1.8  {d22[6]}, [%[v]]!\n"
				"vst1.8  {d20[6]}, [%[y]]!\n"
			".else\n"
				".error \"unsupported macroblock size\"\n"
			".endif\n"
		".endm\n"
		"vld1.16 {d0, d1, d2, d3}, [%[c]]\n" /* load constants */
		"subs        %[n], %[n], #8\n"
		"blt         2f\n"
		"1:\n"
		"do_load 8\n"
		"do_rgb_to_yuv\n"
		"do_store 8\n"
		"subs        %[n], %[n], #8\n"
		"bge         1b\n"
		"beq         8f\n"
		"2:\n"
		"tst         %[n], #4\n"
		"beq         3f\n"
		"do_load 4\n"
		"3:\n"
		"tst         %[n], #2\n"
		"beq         4f\n"
		"do_load 2\n"
		"4:\n"
		"tst         %[n], #1\n"
		"beq         5f\n"
		"do_load 1\n"
		"5:\n"
		"do_rgb_to_yuv\n"
		"tst         %[n], #4\n"
		"beq         6f\n"
		"do_store 4\n"
		"6:\n"
		"tst         %[n], #2\n"
		"beq         7f\n"
		"do_store 2\n"
		"7:\n"
		"tst         %[n], #1\n"
		"beq         8f\n"
		"do_store 1\n"
		"8:\n"
		".purgem do_load\n"
		".purgem do_rgb_to_yuv\n"
		".purgem do_store\n"
		: [rgb] "+&r" (rgb), [y] "+&r" (y), [u] "+&r" (u), [v] "+&r" (v),
		[n] "+&r" (n)
		: [c] "r" (&c[0])
		: "cc", "memory",
		"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
		"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
		"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
		"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
}

METHODDEF(void)
rgb_ycc_convert_neon (j_compress_ptr cinfo,
		JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		JDIMENSION output_row, int num_rows)

{
	register JSAMPROW inptr;
	register JSAMPROW outptr0, outptr1, outptr2;
	JDIMENSION num_cols = cinfo->image_width;
	while (--num_rows >= 0) {
		inptr = *input_buf++;
		outptr0 = output_buf[0][output_row];
		outptr1 = output_buf[1][output_row];
		outptr2 = output_buf[2][output_row];
		output_row++;
		rgb24_to_yuv444_neon(inptr, outptr0, outptr1, outptr2, num_cols);
	}
}

LOCAL(void)
rgb32_to_yuv444_neon (unsigned char *rgb,
			unsigned char *y, unsigned char *u, unsigned char *v,
			  		      int n)
{
	const static unsigned short c[16] = {
	FIX(0.29900), FIX(0.58700), FIX(0.11400), /*  d0[0],  d0[1],  d0[2] */
	FIX(0.16874), FIX(0.33126), FIX(0.50000), /* -d0[3], -d1[0],  d1[1] */
	FIX(0.41869), FIX(0.08131),               /*  d1[1], -d1[2], -d1[3] */
	0x7FFF, 128, 0x7FFF, 128, 0x7FFF, 128, 0x7FFF, 128
	};
#if !(__APPLE__)
	asm volatile(".fpu neon\n");
#endif	
	asm volatile (
		/* r = d10, g = d11, b = d12 */
		".macro do_rgb_to_yuv\n"
		"   vmovl.u8   q2, d10\n" /* r = { d4, d5 } */
		"   vmovl.u8   q3, d11\n" /* g = { d6, d7 } */
		"   vmovl.u8   q4, d12\n" /* b = { d8, d9 } */
		
		"   vmull.u16   q10, d4, d0[0]\n"
		"   vmlal.u16   q10, d6, d0[1]\n"
		"   vmlal.u16   q10, d8, d0[2]\n"
		"   vmull.u16   q11, d5, d0[0]\n"
		"   vmlal.u16   q11, d7, d0[1]\n"
		"   vmlal.u16   q11, d9, d0[2]\n"
		
		"   vmov.u32    q12, q1\n"
		"   vmov.u32    q13, q1\n"
		"   vmlsl.u16   q12, d4, d0[3]\n"
		"   vmlsl.u16   q12, d6, d1[0]\n"
		"   vmlal.u16   q12, d8, d1[1]\n"
		"   vmlsl.u16   q13, d5, d0[3]\n"
		"   vmlsl.u16   q13, d7, d1[0]\n"
		"   vmlal.u16   q13, d9, d1[1]\n"
		
		"   vmov.u32    q14, q1\n"
		"   vmov.u32    q15, q1\n"
		"   vmlal.u16   q14, d4, d1[1]\n"
		"   vmlsl.u16   q14, d6, d1[2]\n"
		"   vmlsl.u16   q14, d8, d1[3]\n"
		"   vmlal.u16   q15, d5, d1[1]\n"
		"   vmlsl.u16   q15, d7, d1[2]\n"
		"   vmlsl.u16   q15, d9, d1[3]\n"

		"   vrshrn.u32  d20, q10, #16\n"
		"   vrshrn.u32  d21, q11, #16\n"
		"   vshrn.u32   d24, q12, #16\n"
		"   vshrn.u32   d25, q13, #16\n"
		"   vshrn.u32   d28, q14, #16\n"
		"   vshrn.u32   d29, q15, #16\n"

		"   vmovn.u16   d20, q10\n"      /* d20 = y */
		"   vmovn.u16   d21, q12\n"      /* d21 = u */
		"   vmovn.u16   d22, q14\n"      /* d22 = v */
		".endm\n"
		".macro do_load size\n"
			".if \\size == 8\n"
				"vld4.8 {d10, d11, d12, d13}, [%[rgb]]!\n"
				"pld [%[rgb], #128]\n"
			".elseif \\size == 4\n"
				"vld4.8 {d10[0], d11[0], d12[0], d13[0]}, [%[rgb]]!\n"
				"vld4.8 {d10[1], d11[1], d12[1], d13[1]}, [%[rgb]]!\n"
				"vld4.8 {d10[2], d11[2], d12[2], d13[2]}, [%[rgb]]!\n"
				"vld4.8 {d10[3], d11[3], d12[3], d13[3]}, [%[rgb]]!\n"
			".elseif \\size == 2\n"
				"vld4.8 {d10[4], d11[4], d12[4], d13[4]}, [%[rgb]]!\n"
				"vld4.8 {d10[5], d11[5], d12[5], d13[5]}, [%[rgb]]!\n"
			".elseif \\size == 1\n"
				"vld4.8 {d10[6], d11[6], d12[6], d13[6]}, [%[rgb]]!\n"
			".else\n"
				".error \"unsupported macroblock size\"\n"
			".endif\n"
		".endm\n"
		".macro do_store size\n"
			".if \\size == 8\n"
				"vst1.8  {d21}, [%[u]]!\n"
				"vst1.8  {d22}, [%[v]]!\n"
				"vst1.8  {d20}, [%[y]]!\n"
			".elseif \\size == 4\n"
				"vst1.8  {d21[0]}, [%[u]]!\n"
				"vst1.8  {d21[1]}, [%[u]]!\n"
				"vst1.8  {d21[2]}, [%[u]]!\n"
				"vst1.8  {d21[3]}, [%[u]]!\n"
				"vst1.8  {d22[0]}, [%[v]]!\n"
				"vst1.8  {d22[1]}, [%[v]]!\n"
				"vst1.8  {d22[2]}, [%[v]]!\n"
				"vst1.8  {d22[3]}, [%[v]]!\n"
				"vst1.8  {d20[0]}, [%[y]]!\n"
				"vst1.8  {d20[1]}, [%[y]]!\n"
				"vst1.8  {d20[2]}, [%[y]]!\n"
				"vst1.8  {d20[3]}, [%[y]]!\n"
			".elseif \\size == 2\n"
				"vst1.8  {d21[4]}, [%[u]]!\n"
				"vst1.8  {d21[5]}, [%[u]]!\n"
				"vst1.8  {d22[4]}, [%[v]]!\n"
				"vst1.8  {d22[5]}, [%[v]]!\n"
				"vst1.8  {d20[4]}, [%[y]]!\n"
				"vst1.8  {d20[5]}, [%[y]]!\n"
			".elseif \\size == 1\n"
				"vst1.8  {d21[6]}, [%[u]]!\n"
				"vst1.8  {d22[6]}, [%[v]]!\n"
				"vst1.8  {d20[6]}, [%[y]]!\n"
			".else\n"
				".error \"unsupported macroblock size\"\n"
			".endif\n"
		".endm\n"
		"vld1.16 {d0, d1, d2, d3}, [%[c]]\n" /* load constants */
		"subs        %[n], %[n], #8\n"
		"blt         2f\n"
		"1:\n"
		"do_load 8\n"
		"do_rgb_to_yuv\n"
		"do_store 8\n"
		"subs        %[n], %[n], #8\n"
		"bge         1b\n"
		"beq         8f\n"
		"2:\n"
		"tst         %[n], #4\n"
		"beq         3f\n"
		"do_load 4\n"
		"3:\n"
		"tst         %[n], #2\n"
		"beq         4f\n"
		"do_load 2\n"
		"4:\n"
		"tst         %[n], #1\n"
		"beq         5f\n"
		"do_load 1\n"
		"5:\n"
		"do_rgb_to_yuv\n"
		"tst         %[n], #4\n"
		"beq         6f\n"
		"do_store 4\n"
		"6:\n"
		"tst         %[n], #2\n"
		"beq         7f\n"
		"do_store 2\n"
		"7:\n"
		"tst         %[n], #1\n"
		"beq         8f\n"
		"do_store 1\n"
		"8:\n"
		".purgem do_load\n"
		".purgem do_rgb_to_yuv\n"
		".purgem do_store\n"
		: [rgb] "+&r" (rgb), [y] "+&r" (y), [u] "+&r" (u), [v] "+&r" (v),
		[n] "+&r" (n)
		: [c] "r" (&c[0])
		: "cc", "memory",
		"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
		"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
		"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
		"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
}

METHODDEF(void)
rgba_ycc_convert_neon (j_compress_ptr cinfo,
		JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		JDIMENSION output_row, int num_rows)

{
	register JSAMPROW inptr;
	register JSAMPROW outptr0, outptr1, outptr2;
	JDIMENSION num_cols = cinfo->image_width;
	while (--num_rows >= 0) {
		inptr = *input_buf++;
		outptr0 = output_buf[0][output_row];
		outptr1 = output_buf[1][output_row];
		outptr2 = output_buf[2][output_row];
		output_row++;
		rgb32_to_yuv444_neon(inptr, outptr0, outptr1, outptr2, num_cols);
	}
}
#endif


METHODDEF(void)
rgb_ycc_convert (j_compress_ptr cinfo,
		 JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		 JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      r = GETJSAMPLE(inptr[RGB_RED]);
      g = GETJSAMPLE(inptr[RGB_GREEN]);
      b = GETJSAMPLE(inptr[RGB_BLUE]);
      inptr += RGB_PIXELSIZE;
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT_SHIFT macro.
       */
      /* Y */
      outptr0[col] = (JSAMPLE)
		((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		 >> SCALEBITS);
      /* Cb */
      outptr1[col] = (JSAMPLE)
		((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
		 >> SCALEBITS);
      /* Cr */
      outptr2[col] = (JSAMPLE)
		((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
		 >> SCALEBITS);
    }
  }
}

METHODDEF(void)
rgba_ycc_convert (j_compress_ptr cinfo,
                 JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                 JDIMENSION output_row, int num_rows)
{
    my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
    register int r, g, b;
    register INT32 * ctab = cconvert->rgb_ycc_tab;
    register JSAMPROW inptr;
    register JSAMPROW outptr0, outptr1, outptr2;
    register JDIMENSION col;
    JDIMENSION num_cols = cinfo->image_width;
    
    while (--num_rows >= 0) {
        inptr = *input_buf++;
        outptr0 = output_buf[0][output_row];
        outptr1 = output_buf[1][output_row];
        outptr2 = output_buf[2][output_row];
        output_row++;
        for (col = 0; col < num_cols; col++) {
            r = GETJSAMPLE(inptr[RGB_RED]);
            g = GETJSAMPLE(inptr[RGB_GREEN]);
            b = GETJSAMPLE(inptr[RGB_BLUE]);
            inptr += RGBA_PIXELSIZE;
            /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
             * must be too; we do not need an explicit range-limiting operation.
             * Hence the value being shifted is never negative, and we don't
             * need the general RIGHT_SHIFT macro.
             */
            /* Y */
            outptr0[col] = (JSAMPLE)
            ((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
             >> SCALEBITS);
            /* Cb */
            outptr1[col] = (JSAMPLE)
            ((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
             >> SCALEBITS);
            /* Cr */
            outptr2[col] = (JSAMPLE)
            ((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
             >> SCALEBITS);
        }
    }
}

/**************** Cases other than RGB -> YCbCr **************/


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles RGB->grayscale conversion, which is the same
 * as the RGB->Y portion of RGB->YCbCr.
 * We assume rgb_ycc_start has been called (we only use the Y tables).
 */

METHODDEF(void)
rgb_gray_convert (j_compress_ptr cinfo,
		  JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		  JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr = output_buf[0][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      r = GETJSAMPLE(inptr[RGB_RED]);
      g = GETJSAMPLE(inptr[RGB_GREEN]);
      b = GETJSAMPLE(inptr[RGB_BLUE]);
      inptr += RGB_PIXELSIZE;
      /* Y */
      outptr[col] = (JSAMPLE)
		((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		 >> SCALEBITS);
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles Adobe-style CMYK->YCCK conversion,
 * where we convert R=1-C, G=1-M, and B=1-Y to YCbCr using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume rgb_ycc_start has been called.
 */

METHODDEF(void)
cmyk_ycck_convert (j_compress_ptr cinfo,
		   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		   JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2, outptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    outptr3 = output_buf[3][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      r = MAXJSAMPLE - GETJSAMPLE(inptr[0]);
      g = MAXJSAMPLE - GETJSAMPLE(inptr[1]);
      b = MAXJSAMPLE - GETJSAMPLE(inptr[2]);
      /* K passes through as-is */
      outptr3[col] = inptr[3];	/* don't need GETJSAMPLE here */
      inptr += 4;
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT_SHIFT macro.
       */
      /* Y */
      outptr0[col] = (JSAMPLE)
		((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		 >> SCALEBITS);
      /* Cb */
      outptr1[col] = (JSAMPLE)
		((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
		 >> SCALEBITS);
      /* Cr */
      outptr2[col] = (JSAMPLE)
		((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
		 >> SCALEBITS);
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles grayscale output with no conversion.
 * The source can be either plain grayscale or YCbCr (since Y == gray).
 */

METHODDEF(void)
grayscale_convert (j_compress_ptr cinfo,
		   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		   JDIMENSION output_row, int num_rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;
  int instride = cinfo->input_components;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr = output_buf[0][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      outptr[col] = inptr[0];	/* don't need GETJSAMPLE() here */
      inptr += instride;
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles multi-component colorspaces without conversion.
 * We assume input_components == num_components.
 */

METHODDEF(void)
null_convert (j_compress_ptr cinfo,
	      JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
	      JDIMENSION output_row, int num_rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  register int ci;
  int nc = cinfo->num_components;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    /* It seems fastest to make a separate pass for each component. */
    for (ci = 0; ci < nc; ci++) {
      inptr = *input_buf;
      outptr = output_buf[ci][output_row];
      for (col = 0; col < num_cols; col++) {
	outptr[col] = inptr[ci]; /* don't need GETJSAMPLE() here */
	inptr += nc;
      }
    }
    input_buf++;
    output_row++;
  }
}


/*
 * Empty method for start_pass.
 */

METHODDEF(void)
null_method (j_compress_ptr cinfo)
{
  /* no work needed */
}


/*
 * Module initialization routine for input colorspace conversion.
 */

GLOBAL(void)
jinit_color_converter (j_compress_ptr cinfo)
{
  my_cconvert_ptr cconvert;

  cconvert = (my_cconvert_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_color_converter));
  cinfo->cconvert = (struct jpeg_color_converter *) cconvert;
  /* set start_pass to null method until we find out differently */
  cconvert->pub.start_pass = null_method;

  /* Make sure input_components agrees with in_color_space */
  switch (cinfo->in_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->input_components != 1)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  case JCS_RGB:
#if RGB_PIXELSIZE != 3
    if (cinfo->input_components != RGB_PIXELSIZE)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;
#endif /* else share code with YCbCr */
  case JCS_RGBA:

  case JCS_YCbCr:
    if (cinfo->input_components != 3)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  case JCS_CMYK:
  case JCS_YCCK:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  default:			/* JCS_UNKNOWN can be anything */
    if (cinfo->input_components < 1)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;
  }

  /* Check num_components, set conversion method based on requested space */
  switch (cinfo->jpeg_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_GRAYSCALE)
      cconvert->pub.color_convert = grayscale_convert;
    else if ((cinfo->in_color_space == JCS_RGB) || (cinfo->in_color_space == JCS_RGBA)) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = rgb_gray_convert;
    } else if (cinfo->in_color_space == JCS_YCbCr)
      cconvert->pub.color_convert = grayscale_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_RGB:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_RGB && RGB_PIXELSIZE == 3)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_RGB) {
#if KINOMA_DCT_IFAST_SUPPORTED 
    int features = FskHardwareGetARMCPU_All();
    if(features==FSK_ARCH_ARM_V7)
      cconvert->pub.color_convert = rgb_ycc_convert_neon;
    else
#endif
      cconvert->pub.color_convert = rgb_ycc_convert;
      cconvert->pub.start_pass = rgb_ycc_start;
    } else if (cinfo->in_color_space == JCS_RGBA) {
#if KINOMA_DCT_IFAST_SUPPORTED 
    	int features = FskHardwareGetARMCPU_All();
    	if(features==FSK_ARCH_ARM_V7)
      		cconvert->pub.color_convert = rgba_ycc_convert_neon;
    	else
#endif
        cconvert->pub.color_convert = rgba_ycc_convert;
        cconvert->pub.start_pass = rgb_ycc_start;
    } else if (cinfo->in_color_space == JCS_YCbCr)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_CMYK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_CMYK)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_YCCK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_CMYK) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = cmyk_ycck_convert;
    } else if (cinfo->in_color_space == JCS_YCCK)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  default:			/* allow null conversion of JCS_UNKNOWN */
    if (cinfo->jpeg_color_space != cinfo->in_color_space ||
	cinfo->num_components != cinfo->input_components)
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    cconvert->pub.color_convert = null_convert;
    break;
  }
}
