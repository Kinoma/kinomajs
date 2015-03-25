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
#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"
#include "kinoma_utilities.h"

IppStatus (__STDCALL *ippiDCT8x8Fwd_8u16s_C1R_universal) 		( const Ipp8u* pSrc, int srcStep, Ipp16s* pDst)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Fwd_16s_C1R_universal) 			(const Ipp16s* pSrc, int srcStep, Ipp16s* pDst )=NULL;
IppStatus (__STDCALL *ippiDCT8x8Fwd_16s_C1I_universal) 			(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_16s_C1I_universal)			(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_4x4_16s_C1I_universal)		(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_2x2_16s_C1I_universal)		(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_DC_16s_C1I_universal)		(Ipp16s* pSrcDst)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_16s8u_C1R_universal)		(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_4x4_16s8u_C1R_universal)	(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_2x2_16s8u_C1R_universal)	(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)=NULL;
IppStatus (__STDCALL *ippiDCT8x8Inv_DC_16s8u_C1R_universal)	(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)=NULL;

#define KINOMA_DCT_C
//#define INTEL_DCT
//#ifdef __KINOMA_IPP_ARM_V5__
//#define KINOMA_DCT_ARM
//#endif

/************************** Fast IDCT algorithm***********************/

/**********************************************************/
/* inverse two dimensional DCT, Chen-Wang algorithm       */
/* (cf. IEEE ASSP-32, pp. 803-816, Aug. 1984)             */
/* 32-bit integer arithmetic (8 bit coefficients)         */
/* 11 mults, 29 adds per DCT                              */
/*                                      sE, 18.8.91       */
/**********************************************************/
/* coefficients extended to 12 bit for IEEE1180-1990      */
/* compliance                           sE,  2.1.94       */
/**********************************************************/

/* this code assumes >> to be a two's-complement arithmetic */
/* right shift: (-2)>>1 == -1 , (-3)>>1 == -2               */

 static const short Clip[1024] =	/* clipping table */
{
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
 -256, -256, -256, -256, -256, -255, -254, -253, -252, -251, -250, -249,
 -248, -247, -246, -245, -244, -243, -242, -241, -240, -239, -238, -237,
 -236, -235, -234, -233, -232, -231, -230, -229, -228, -227, -226, -225,
 -224, -223, -222, -221, -220, -219, -218, -217, -216, -215, -214, -213,
 -212, -211, -210, -209, -208, -207, -206, -205, -204, -203, -202, -201,
 -200, -199, -198, -197, -196, -195, -194, -193, -192, -191, -190, -189,
 -188, -187, -186, -185, -184, -183, -182, -181, -180, -179, -178, -177,
 -176, -175, -174, -173, -172, -171, -170, -169, -168, -167, -166, -165,
 -164, -163, -162, -161, -160, -159, -158, -157, -156, -155, -154, -153,
 -152, -151, -150, -149, -148, -147, -146, -145, -144, -143, -142, -141,
 -140, -139, -138, -137, -136, -135, -134, -133, -132, -131, -130, -129,
 -128, -127, -126, -125, -124, -123, -122, -121, -120, -119, -118, -117,
 -116, -115, -114, -113, -112, -111, -110, -109, -108, -107, -106, -105,
 -104, -103, -102, -101, -100,  -99,  -98,  -97,  -96,  -95,  -94,  -93,
  -92,  -91,  -90,  -89,  -88,  -87,  -86,  -85,  -84,  -83,  -82,  -81,
  -80,  -79,  -78,  -77,  -76,  -75,  -74,  -73,  -72,  -71,  -70,  -69,
  -68,  -67,  -66,  -65,  -64,  -63,  -62,  -61,  -60,  -59,  -58,  -57,
  -56,  -55,  -54,  -53,  -52,  -51,  -50,  -49,  -48,  -47,  -46,  -45,
  -44,  -43,  -42,  -41,  -40,  -39,  -38,  -37,  -36,  -35,  -34,  -33,
  -32,  -31,  -30,  -29,  -28,  -27,  -26,  -25,  -24,  -23,  -22,  -21,
  -20,  -19,  -18,  -17,  -16,  -15,  -14,  -13,  -12,  -11,  -10,   -9,
   -8,   -7,   -6,   -5,   -4,   -3,   -2,   -1,    0,    1,    2,    3,
    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,
   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,
   40,   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,
   52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   62,   63,
   64,   65,   66,   67,   68,   69,   70,   71,   72,   73,   74,   75,
   76,   77,   78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
   88,   89,   90,   91,   92,   93,   94,   95,   96,   97,   98,   99,
  100,  101,  102,  103,  104,  105,  106,  107,  108,  109,  110,  111,
  112,  113,  114,  115,  116,  117,  118,  119,  120,  121,  122,  123,
  124,  125,  126,  127,  128,  129,  130,  131,  132,  133,  134,  135,
  136,  137,  138,  139,  140,  141,  142,  143,  144,  145,  146,  147,
  148,  149,  150,  151,  152,  153,  154,  155,  156,  157,  158,  159,
  160,  161,  162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
  172,  173,  174,  175,  176,  177,  178,  179,  180,  181,  182,  183,
  184,  185,  186,  187,  188,  189,  190,  191,  192,  193,  194,  195,
  196,  197,  198,  199,  200,  201,  202,  203,  204,  205,  206,  207,
  208,  209,  210,  211,  212,  213,  214,  215,  216,  217,  218,  219,
  220,  221,  222,  223,  224,  225,  226,  227,  228,  229,  230,  231,
  232,  233,  234,  235,  236,  237,  238,  239,  240,  241,  242,  243,
  244,  245,  246,  247,  248,  249,  250,  251,  252,  253,  254,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
  255,  255,  255,  255
};
static const short *pClip = Clip + 512;


#define WT1 2841                 /* 2048*sqrt(2)*cos(1*pi/16) */
#define WT2 2676                 /* 2048*sqrt(2)*cos(2*pi/16) */
#define WT3 2408                 /* 2048*sqrt(2)*cos(3*pi/16) */
#define WT5 1609                 /* 2048*sqrt(2)*cos(5*pi/16) */
#define WT6 1108                 /* 2048*sqrt(2)*cos(6*pi/16) */
#define WT7 565                  /* 2048*sqrt(2)*cos(7*pi/16) */

/* row (horizontal) IDCT
 * 
 * 7                       pi         1 dst[k] = sum c[l] * src[l] * cos( -- *
 * ( k + - ) * l ) l=0                      8          2
 * 
 * where: c[0]    = 128 c[1..7] = 128*sqrt(2) 
*/
void IDCT_ROW_kinoma (short *pSrcDst)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

	/* shortcut */
	/*if (!((tmp1 = pSrcDst[4] << 11) | (tmp2 = pSrcDst[6]) | (tmp3 = pSrcDst[2]) |
		(tmp4 = pSrcDst[1]) | (tmp5 = pSrcDst[7]) | (tmp6 = pSrcDst[5]) | (tmp7 = pSrcDst[3])))*/
	if (!(
		(tmp4 = pSrcDst[1]) | (tmp3 = pSrcDst[2]) | (tmp7 = pSrcDst[3])|(tmp1 = pSrcDst[4] << 11) | (tmp6 = pSrcDst[5]) | (tmp2 = pSrcDst[6]) | (tmp5 = pSrcDst[7]) ))
	{
		pSrcDst[0] = pSrcDst[1] = pSrcDst[2] = pSrcDst[3] = pSrcDst[4] = pSrcDst[5] = pSrcDst[6] = pSrcDst[7] = pSrcDst[0] << 3;
		return;
	}
	tmp0 = (pSrcDst[0] << 11) + 128;    /* for proper rounding in the fourth stage */

	/* first stage */
	tmp8 = WT7 * (tmp4 + tmp5);
	tmp4 = tmp8 + (WT1 - WT7) * tmp4;
	tmp5 = tmp8 - (WT1 + WT7) * tmp5;
	tmp8 = WT3 * (tmp6 + tmp7);
	tmp6 = tmp8 - (WT3 - WT5) * tmp6;
	tmp7 = tmp8 - (WT3 + WT5) * tmp7;

	/* second stage */
	tmp8 = tmp0 + tmp1;
	tmp0 -= tmp1;
	tmp1 = WT6 * (tmp3 + tmp2);
	tmp2 = tmp1 - (WT2 + WT6) * tmp2;
	tmp3 = tmp1 + (WT2 - WT6) * tmp3;
	tmp1 = tmp4 + tmp6;
	tmp4 -= tmp6;
	tmp6 = tmp5 + tmp7;
	tmp5 -= tmp7;

	/* third stage */
	tmp7 = tmp8 + tmp3;
	tmp8 -= tmp3;
	tmp3 = tmp0 + tmp2;
	tmp0 -= tmp2;
	tmp2 = (181 * (tmp4 + tmp5) + 128) >> 8;
	tmp4 = (181 * (tmp4 - tmp5) + 128) >> 8;

	/* fourth stage */
	pSrcDst[0] = (tmp7 + tmp1) >> 8;
	pSrcDst[1] = (tmp3 + tmp2) >> 8;
	pSrcDst[2] = (tmp0 + tmp4) >> 8;
	pSrcDst[3] = (tmp8 + tmp6) >> 8;
	pSrcDst[4] = (tmp8 - tmp6) >> 8;
	pSrcDst[5] = (tmp0 - tmp4) >> 8;
	pSrcDst[6] = (tmp3 - tmp2) >> 8;
	pSrcDst[7] = (tmp7 - tmp1) >> 8;
}

/* column (vertical) IDCT
* 
* 7                         pi         1 dst[8*k] = sum c[l] * src[8*l] *
* cos( -- * ( k + - ) * l ) l=0                        8          2
* 
* where: c[0]    = 1/1024 c[1..7] = (1/1024)*sqrt(2) 
*/
void IDCT_COL_kinoma(short *pSrcDst)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

	/* shortcut */
	/*if (!((tmp1 = (pSrcDst[32] << 8)) | (tmp2 = pSrcDst[48]) | (tmp3 = pSrcDst[16]) |
		(tmp4 = pSrcDst[8]) | (tmp5 = pSrcDst[56]) | (tmp6 = pSrcDst[40]) | (tmp7 = pSrcDst[24])))*/
	if (!(
		(tmp4 = pSrcDst[8]) | (tmp3 = pSrcDst[16]) | (tmp7 = pSrcDst[24])|(tmp1 = (pSrcDst[32] << 8)) | (tmp2 = pSrcDst[48]) | (tmp5 = pSrcDst[56]) | (tmp6 = pSrcDst[40]) ))

	{
		pSrcDst[0] = pSrcDst[8] = pSrcDst[16] = pSrcDst[24] = pSrcDst[32] = pSrcDst[40] = pSrcDst[48] = pSrcDst[56] =
			pClip[(pSrcDst[0] + 32) >> 6];
		return;
	}
	tmp0 = (pSrcDst[0] << 8) + 8192;

	/* first stage */
	tmp8 = WT7 * (tmp4 + tmp5) + 4;
	tmp4 = (tmp8 + (WT1 - WT7) * tmp4) >> 3;
	tmp5 = (tmp8 - (WT1 + WT7) * tmp5) >> 3;
	tmp8 = WT3 * (tmp6 + tmp7) + 4;
	tmp6 = (tmp8 - (WT3 - WT5) * tmp6) >> 3;
	tmp7 = (tmp8 - (WT3 + WT5) * tmp7) >> 3;

	/* second stage */
	tmp8 = tmp0 + tmp1;
	tmp0 -= tmp1;
	tmp1 = WT6 * (tmp3 + tmp2) + 4;
	tmp2 = (tmp1 - (WT2 + WT6) * tmp2) >> 3;
	tmp3 = (tmp1 + (WT2 - WT6) * tmp3) >> 3;
	tmp1 = tmp4 + tmp6;
	tmp4 -= tmp6;
	tmp6 = tmp5 + tmp7;
	tmp5 -= tmp7;

	/* third stage */
	tmp7 = tmp8 + tmp3;
	tmp8 -= tmp3;
	tmp3 = tmp0 + tmp2;
	tmp0 -= tmp2;
	tmp2 = (181 * (tmp4 + tmp5) + 128) >> 8;
	tmp4 = (181 * (tmp4 - tmp5) + 128) >> 8;

	/* fourth stage */
	pSrcDst[0] = pClip[(tmp7 + tmp1) >> 14];
	pSrcDst[8] = pClip[(tmp3 + tmp2) >> 14];
	pSrcDst[16] = pClip[(tmp0 + tmp4) >> 14];
	pSrcDst[24] = pClip[(tmp8 + tmp6) >> 14];
	pSrcDst[32] = pClip[(tmp8 - tmp6) >> 14];
	pSrcDst[40] = pClip[(tmp0 - tmp4) >> 14];
	pSrcDst[48] = pClip[(tmp3 - tmp2) >> 14];
	pSrcDst[56] = pClip[(tmp7 - tmp1) >> 14];
}

/*-------------------- Fast IDCT algorithm end-----------------------*/



/***********************FDCT row and col******************************/
#define USE_ACCURATE_ROUNDING

#define RIGHT_SHIFT(x, shft)  ((x) >> (shft))

#ifdef USE_ACCURATE_ROUNDING
#define ONE ((int) 1)
#define DESCALE(x, n)  RIGHT_SHIFT((x) + (ONE << ((n) - 1)), n)
#else
#define DESCALE(x, n)  RIGHT_SHIFT(x, n)
#endif

#define PASS1_BITS  2

#define FIX_0_298631336  ((int)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((int)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((int)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((int)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((int)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((int)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((int) 12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((int) 15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((int) 16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((int) 16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((int) 20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((int) 25172)	/* FIX(3.072711026) */

void FDCT_ROW(short *pSrcDst)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z1, z2, z3, z4, z5;

	tmp0 = pSrcDst[0] + pSrcDst[7];
	tmp7 = pSrcDst[0] - pSrcDst[7];
	tmp1 = pSrcDst[1] + pSrcDst[6];
	tmp6 = pSrcDst[1] - pSrcDst[6];
	tmp2 = pSrcDst[2] + pSrcDst[5];
	tmp5 = pSrcDst[2] - pSrcDst[5];
	tmp3 = pSrcDst[3] + pSrcDst[4];
	tmp4 = pSrcDst[3] - pSrcDst[4];

	/* Even part per LL&M figure 1 --- note that published figure is faulty;
	* rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
	*/

	tmp10 = tmp0 + tmp3;
	tmp13 = tmp0 - tmp3;
	tmp11 = tmp1 + tmp2;
	tmp12 = tmp1 - tmp2;

	pSrcDst[0] = (tmp10 + tmp11) << PASS1_BITS;
	pSrcDst[4] = (tmp10 - tmp11) << PASS1_BITS;

	z1 = (tmp12 + tmp13) * FIX_0_541196100;
	pSrcDst[2] =
		DESCALE(z1 + tmp13 * FIX_0_765366865, 11);
	pSrcDst[6] =
		DESCALE(z1 + tmp12 * (-FIX_1_847759065), 11);

	/* Odd part per figure 8 --- note paper omits factor of sqrt(2).
	* cK represents cos(K*pi/16).
	* i0..i3 in the paper are tmp4..tmp7 here.
	*/

	z1 = tmp4 + tmp7;
	z2 = tmp5 + tmp6;
	z3 = tmp4 + tmp6;
	z4 = tmp5 + tmp7;
	z5 = (z3 + z4) * FIX_1_175875602;	/* sqrt(2) * c3 */

	tmp4 *= FIX_0_298631336;	/* sqrt(2) * (-c1+c3+c5-c7) */
	tmp5 *= FIX_2_053119869;	/* sqrt(2) * ( c1+c3-c5+c7) */
	tmp6 *= FIX_3_072711026;	/* sqrt(2) * ( c1+c3+c5-c7) */
	tmp7 *= FIX_1_501321110;	/* sqrt(2) * ( c1+c3-c5-c7) */
	z1 *= -FIX_0_899976223;	/* sqrt(2) * (c7-c3) */
	z2 *= -FIX_2_562915447;	/* sqrt(2) * (-c1-c3) */
	z3 *= -FIX_1_961570560;	/* sqrt(2) * (-c3-c5) */
	z4 *= -FIX_0_390180644;	/* sqrt(2) * (c5-c3) */

	z3 += z5;
	z4 += z5;

	pSrcDst[7] = DESCALE(tmp4 + z1 + z3, 11);
	pSrcDst[5] = DESCALE(tmp5 + z2 + z4, 11);
	pSrcDst[3] = DESCALE(tmp6 + z2 + z3, 11);
	pSrcDst[1] = DESCALE(tmp7 + z1 + z4, 11);

}

void FDCT_COL(short *pSrcDst)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z1, z2, z3, z4, z5;

	tmp0 = pSrcDst[0] + pSrcDst[56];
	tmp7 = pSrcDst[0] - pSrcDst[56];
	tmp1 = pSrcDst[8] + pSrcDst[48];
	tmp6 = pSrcDst[8] - pSrcDst[48];
	tmp2 = pSrcDst[16] + pSrcDst[40];
	tmp5 = pSrcDst[16] - pSrcDst[40];
	tmp3 = pSrcDst[24] + pSrcDst[32];
	tmp4 = pSrcDst[24] - pSrcDst[32];

	/* Even part per LL&M figure 1 --- note that published figure is faulty;
	* rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
	*/

	tmp10 = tmp0 + tmp3;
	tmp13 = tmp0 - tmp3;
	tmp11 = tmp1 + tmp2;
	tmp12 = tmp1 - tmp2;

	pSrcDst[0] = DESCALE(tmp10 + tmp11, PASS1_BITS);
	pSrcDst[32] = DESCALE(tmp10 - tmp11, PASS1_BITS);

	z1 = (tmp12 + tmp13) * FIX_0_541196100;
	pSrcDst[16] =
		DESCALE(z1 + tmp13 * FIX_0_765366865, 15);
	pSrcDst[48] =
		DESCALE(z1 + tmp12 * (-FIX_1_847759065), 15);

	/* Odd part per figure 8 --- note paper omits factor of sqrt(2).
	* cK represents cos(K*pi/16).
	* i0..i3 in the paper are tmp4..tmp7 here.
	*/

	z1 = tmp4 + tmp7;
	z2 = tmp5 + tmp6;
	z3 = tmp4 + tmp6;
	z4 = tmp5 + tmp7;
	z5 = (z3 + z4) * FIX_1_175875602;	/* sqrt(2) * c3 */

	tmp4 *= FIX_0_298631336;	/* sqrt(2) * (-c1+c3+c5-c7) */
	tmp5 *= FIX_2_053119869;	/* sqrt(2) * ( c1+c3-c5+c7) */
	tmp6 *= FIX_3_072711026;	/* sqrt(2) * ( c1+c3+c5-c7) */
	tmp7 *= FIX_1_501321110;	/* sqrt(2) * ( c1+c3-c5-c7) */
	z1 *= -FIX_0_899976223;	/* sqrt(2) * (c7-c3) */
	z2 *= -FIX_2_562915447;	/* sqrt(2) * (-c1-c3) */
	z3 *= -FIX_1_961570560;	/* sqrt(2) * (-c3-c5) */
	z4 *= -FIX_0_390180644;	/* sqrt(2) * (c5-c3) */

	z3 += z5;
	z4 += z5;

	pSrcDst[56] = DESCALE(tmp4 + z1 + z3, 15);
	pSrcDst[40] = DESCALE(tmp5 + z2 + z4, 15);
	pSrcDst[24] = DESCALE(tmp6 + z2 + z3, 15);
	pSrcDst[8] = DESCALE(tmp7 + z1 + z4, 15);
}
/*---------------------FDCT row and col end--------------------------*/





#if 0
#define W1 2841 /* 2048*sqrt (2)*cos (1*pi/16) */
#define W2 2676 /* 2048*sqrt (2)*cos (2*pi/16) */
#define W3 2408 /* 2048*sqrt (2)*cos (3*pi/16) */
#define W4 2048 /* 2048*sqrt (2)*cos (4*pi/16) */
#define W5 1609 /* 2048*sqrt (2)*cos (5*pi/16) */
#define W6 1108 /* 2048*sqrt (2)*cos (6*pi/16) */
#define W7 565  /* 2048*sqrt (2)*cos (7*pi/16) */
#define ROW_SHIFT 8
#define COL_SHIFT 17
#else
#define W1  22725  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W2  21407  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W3  19266  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W4  16383  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W5  12873  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W6  8867   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W7  4520   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define ROW_SHIFT 11
#define COL_SHIFT 20 // 6
#endif


/* signed 16x16 -> 32 multiply add accumulate */
#define MAC16(rt, ra, rb) rt += (ra) * (rb)

/* signed 16x16 -> 32 multiply */
#define MUL16(rt, ra, rb) rt = (ra) * (rb)

static void idctRowCondDC (short * row)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
#ifdef HAVE_FAST_64BIT
        uint64_t temp;
#else
        unsigned long temp;
#endif
	
#ifdef HAVE_FAST_64BIT
#ifdef WORDS_BIGENDIAN
#define ROW0_MASK 0xffff000000000000LL
#else
#define ROW0_MASK 0xffffLL
#endif
        if(sizeof(short)==2){
            if ( ((((uint64_t *)row)[0] & ~ROW0_MASK) |
                  ((uint64_t *)row)[1]) == 0) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                temp += temp << 32;
                ((uint64_t *)row)[0] = temp;
                ((uint64_t *)row)[1] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#else
        if(sizeof(short)==2){
            if (!(((unsigned long*)row)[1] |
                  ((unsigned long*)row)[2] |
                  ((unsigned long*)row)[3] |
                  row[1])) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                ((unsigned long*)row)[0]=((unsigned long*)row)[1] =
                ((unsigned long*)row)[2]=((unsigned long*)row)[3] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#endif

        a0 = (W4 * row[0]) + (1 << (ROW_SHIFT - 1));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        /* no need to optimize : gcc does it */
        a0 += W2 * row[2];
        a1 += W6 * row[2];
        a2 -= W6 * row[2];
        a3 -= W2 * row[2];

        MUL16(b0, W1, row[1]);
        MAC16(b0, W3, row[3]);
        MUL16(b1, W3, row[1]);
        MAC16(b1, -W7, row[3]);
        MUL16(b2, W5, row[1]);
        MAC16(b2, -W1, row[3]);
        MUL16(b3, W7, row[1]);
        MAC16(b3, -W5, row[3]);

#ifdef HAVE_FAST_64BIT
        temp = ((uint64_t*)row)[1];
#else
        temp = ((unsigned long*)row)[2] | ((unsigned long*)row)[3];
#endif
        if (temp != 0) {
            a0 += W4*row[4] + W6*row[6];
            a1 += - W4*row[4] - W2*row[6];
            a2 += - W4*row[4] + W2*row[6];
            a3 += W4*row[4] - W6*row[6];

            MAC16(b0, W5, row[5]);
            MAC16(b0, W7, row[7]);

            MAC16(b1, -W1, row[5]);
            MAC16(b1, -W5, row[7]);

            MAC16(b2, W7, row[5]);
            MAC16(b2, W3, row[7]);

            MAC16(b3, W3, row[5]);
            MAC16(b3, -W1, row[7]);
        }

        row[0] = (a0 + b0) >> ROW_SHIFT;
        row[7] = (a0 - b0) >> ROW_SHIFT;
        row[1] = (a1 + b1) >> ROW_SHIFT;
        row[6] = (a1 - b1) >> ROW_SHIFT;
        row[2] = (a2 + b2) >> ROW_SHIFT;
        row[5] = (a2 - b2) >> ROW_SHIFT;
        row[3] = (a3 + b3) >> ROW_SHIFT;
        row[4] = (a3 - b3) >> ROW_SHIFT;
}


static void idctRowCondDC_4x4 (short * row)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
#ifdef HAVE_FAST_64BIT
        uint64_t temp;
#else
        unsigned long temp;
#endif
	
#ifdef HAVE_FAST_64BIT
#ifdef WORDS_BIGENDIAN
#define ROW0_MASK 0xffff000000000000LL
#else
#define ROW0_MASK 0xffffLL
#endif
        if(sizeof(short)==2){
            if ( ((((uint64_t *)row)[0] & ~ROW0_MASK) |
                  ((uint64_t *)row)[1]) == 0) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                temp += temp << 32;
                ((uint64_t *)row)[0] = temp;
                ((uint64_t *)row)[1] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#else
        if(sizeof(short)==2){
            if (!(((unsigned long*)row)[1] |
                  ((unsigned long*)row)[2] |
                  ((unsigned long*)row)[3] |
                  row[1])) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                ((unsigned long*)row)[0]=((unsigned long*)row)[1] =
                ((unsigned long*)row)[2]=((unsigned long*)row)[3] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#endif

        a0 = (W4 * row[0]) + (1 << (ROW_SHIFT - 1));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        /* no need to optimize : gcc does it */
        a0 += W2 * row[2];
        a1 += W6 * row[2];
        a2 -= W6 * row[2];
        a3 -= W2 * row[2];

        MUL16(b0, W1, row[1]);
        MAC16(b0, W3, row[3]);
        MUL16(b1, W3, row[1]);
        MAC16(b1, -W7, row[3]);
        MUL16(b2, W5, row[1]);
        MAC16(b2, -W1, row[3]);
        MUL16(b3, W7, row[1]);
        MAC16(b3, -W5, row[3]);

        row[0] = (a0 + b0) >> ROW_SHIFT;
        row[7] = (a0 - b0) >> ROW_SHIFT;
        row[1] = (a1 + b1) >> ROW_SHIFT;
        row[6] = (a1 - b1) >> ROW_SHIFT;
        row[2] = (a2 + b2) >> ROW_SHIFT;
        row[5] = (a2 - b2) >> ROW_SHIFT;
        row[3] = (a3 + b3) >> ROW_SHIFT;
        row[4] = (a3 - b3) >> ROW_SHIFT;
}

static void idctRowCondDC_2x2 (short * row)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
#ifdef HAVE_FAST_64BIT
        uint64_t temp;
#else
        unsigned long temp;
#endif
	
#ifdef HAVE_FAST_64BIT
#ifdef WORDS_BIGENDIAN
#define ROW0_MASK 0xffff000000000000LL
#else
#define ROW0_MASK 0xffffLL
#endif
        if(sizeof(short)==2){
            if ( ((((uint64_t *)row)[0] & ~ROW0_MASK) |
                  ((uint64_t *)row)[1]) == 0) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                temp += temp << 32;
                ((uint64_t *)row)[0] = temp;
                ((uint64_t *)row)[1] = temp;
                return;
            }
        }else{
            if (!(row[1])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#else
        if(sizeof(short)==2)
		{
            if (!(row[1])) 
			{
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                ((unsigned long*)row)[0]=((unsigned long*)row)[1] =
                ((unsigned long*)row)[2]=((unsigned long*)row)[3] = temp;
                return;
            }
        }
		else
		{
            if (!row[1]) 
			{
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#endif

        a0 = (W4 * row[0]) + (1 << (ROW_SHIFT - 1));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        MUL16(b0, W1, row[1]);
        MUL16(b1, W3, row[1]);
        MUL16(b2, W5, row[1]);
        MUL16(b3, W7, row[1]);

        row[0] = (a0 + b0) >> ROW_SHIFT;
        row[7] = (a0 - b0) >> ROW_SHIFT;
        row[1] = (a1 + b1) >> ROW_SHIFT;
        row[6] = (a1 - b1) >> ROW_SHIFT;
        row[2] = (a2 + b2) >> ROW_SHIFT;
        row[5] = (a2 - b2) >> ROW_SHIFT;
        row[3] = (a3 + b3) >> ROW_SHIFT;
        row[4] = (a3 - b3) >> ROW_SHIFT;
}

static void idctSparseCol (short * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        MUL16(b0, W1, col[8*1]);
        MUL16(b1, W3, col[8*1]);
        MUL16(b2, W5, col[8*1]);
        MUL16(b3, W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        col[0 ] = ((a0 + b0) >> COL_SHIFT);
        col[8 ] = ((a1 + b1) >> COL_SHIFT);
        col[16] = ((a2 + b2) >> COL_SHIFT);
        col[24] = ((a3 + b3) >> COL_SHIFT);
        col[32] = ((a3 - b3) >> COL_SHIFT);
        col[40] = ((a2 - b2) >> COL_SHIFT);
        col[48] = ((a1 - b1) >> COL_SHIFT);
        col[56] = ((a0 - b0) >> COL_SHIFT);
}

static void idctSparseCol_4x4 (short * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        MUL16(b0, W1, col[8*1]);
        MUL16(b1, W3, col[8*1]);
        MUL16(b2, W5, col[8*1]);
        MUL16(b3, W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        col[0 ] = ((a0 + b0) >> COL_SHIFT);
        col[8 ] = ((a1 + b1) >> COL_SHIFT);
        col[16] = ((a2 + b2) >> COL_SHIFT);
        col[24] = ((a3 + b3) >> COL_SHIFT);
        col[32] = ((a3 - b3) >> COL_SHIFT);
        col[40] = ((a2 - b2) >> COL_SHIFT);
        col[48] = ((a1 - b1) >> COL_SHIFT);
        col[56] = ((a0 - b0) >> COL_SHIFT);
}

static void idctSparseCol_2x2 (short * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        MUL16(b0, W1, col[8*1]);
        MUL16(b1, W3, col[8*1]);
        MUL16(b2, W5, col[8*1]);
        MUL16(b3, W7, col[8*1]);

        col[0 ] = ((a0 + b0) >> COL_SHIFT);
        col[8 ] = ((a1 + b1) >> COL_SHIFT);
        col[16] = ((a2 + b2) >> COL_SHIFT);
        col[24] = ((a3 + b3) >> COL_SHIFT);
        col[32] = ((a3 - b3) >> COL_SHIFT);
        col[40] = ((a2 - b2) >> COL_SHIFT);
        col[48] = ((a1 - b1) >> COL_SHIFT);
        col[56] = ((a0 - b0) >> COL_SHIFT);
}


/*
static void view_block( short *b )
{
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	b += 8;
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	b += 8;
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	b += 8;
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	b += 8;
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	b += 8;
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	b += 8;
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	b += 8;
	printf( "block: %4x, %4x, %4x, %4x, %4x, %4x, %4x, %4x\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],b[7]);
	printf("\n");

}
*/

/*
static void simple_idct(short *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

	//printf( "simple_idct after row\n");
	//view_block(block);

    for(i=0; i<8; i++)
        idctSparseCol(block + i);
}
*/

#if 0
#define IDCT_ROW(a) IDCT_ROW_kinoma(a)
#define IDCT_COL(a) IDCT_COL_kinoma(a)
#else
#define IDCT_ROW(a) idctRowCondDC(a)
#define IDCT_COL(a) idctSparseCol(a)
#endif

#define IDCT_ROW_4x4(a) idctRowCondDC_4x4(a)
#define IDCT_COL_4x4(a) idctSparseCol_4x4(a)

#define IDCT_ROW_2x2(a) idctRowCondDC_2x2(a)
#define IDCT_COL_2x2(a) idctSparseCol_2x2(a)

IppStatus __STDCALL  
ippiDCT8x8Inv_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
#if defined(KINOMA_DCT_C)
	Ipp16s Dst[64], *pTmp;
	Ipp8u *pD;
	int i;
	
	Profile_Start(ippiDCT8x8Inv_16s8u_C1R_c_profile);

	memcpy(Dst,pSrc,64*sizeof(Ipp16s));

	IDCT_ROW(Dst);
	IDCT_ROW(Dst+8);
	IDCT_ROW(Dst+16);
	IDCT_ROW(Dst+24);
	IDCT_ROW(Dst+32);
	IDCT_ROW(Dst+40);
	IDCT_ROW(Dst+48);
	IDCT_ROW(Dst+56);

	IDCT_COL(Dst);
	IDCT_COL(Dst+1);
	IDCT_COL(Dst+2);
	IDCT_COL(Dst+3);
	IDCT_COL(Dst+4);
	IDCT_COL(Dst+5);
	IDCT_COL(Dst+6);
	IDCT_COL(Dst+7);

	pTmp = Dst;
	pD = pDst;
	for (i = 0; i < 8; i++)
	{
		pD[0] = CLIP(pTmp[0]);
		pD[1] = CLIP(pTmp[1]);
		pD[2] = CLIP(pTmp[2]);
		pD[3] = CLIP(pTmp[3]);
		pD[4] = CLIP(pTmp[4]);
		pD[5] = CLIP(pTmp[5]);
		pD[6] = CLIP(pTmp[6]);
		pD[7] = CLIP(pTmp[7]);
		pD += dstStep;
		pTmp += 8;
	}
	
	Profile_End(ippiDCT8x8Inv_16s8u_C1R_c_profile);

#elif defined( INTEL_DCT )
	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s8u_C1R(pSrc, pDst, dstStep );
		return sts;
	}
#elif defined( KINOMA_DCT_ARM )
	{
		IppStatus sts;
		Profile_Start(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
		sts = ippiDCT8x8Inv_16s8u_C1R_arm(pSrc, pDst, dstStep );
		Profile_End(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
		return sts;
	}
#endif
	return ippStsNoErr;
}

IppStatus __STDCALL  
ippiDCT8x8Inv_4x4_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
#if defined(KINOMA_DCT_C)
	Ipp16s Dst[64], *pTmp;
	Ipp8u *pD;
	int i;

	Profile_Start(ippiDCT8x8Inv_16s8u_C1R_c_profile);

	memcpy(Dst,pSrc,64*sizeof(Ipp16s));

	IDCT_ROW_4x4(Dst);
	IDCT_ROW_4x4(Dst+8);
	IDCT_ROW_4x4(Dst+16);
	IDCT_ROW_4x4(Dst+24);

	IDCT_COL_4x4(Dst);
	IDCT_COL_4x4(Dst+1);
	IDCT_COL_4x4(Dst+2);
	IDCT_COL_4x4(Dst+3);
	IDCT_COL_4x4(Dst+4);
	IDCT_COL_4x4(Dst+5);
	IDCT_COL_4x4(Dst+6);
	IDCT_COL_4x4(Dst+7);

	pTmp = Dst;
	pD = pDst;
	for (i = 0; i < 8; i++)
	{
		pD[0] = CLIP(pTmp[0]);
		pD[1] = CLIP(pTmp[1]);
		pD[2] = CLIP(pTmp[2]);
		pD[3] = CLIP(pTmp[3]);
		pD[4] = CLIP(pTmp[4]);
		pD[5] = CLIP(pTmp[5]);
		pD[6] = CLIP(pTmp[6]);
		pD[7] = CLIP(pTmp[7]);
		pD += dstStep;
		pTmp += 8;
	}
	
	Profile_End(ippiDCT8x8Inv_16s8u_C1R_c_profile);

#elif defined( INTEL_DCT )
	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s8u_C1R(pSrc, pDst, dstStep );
		return sts;
	}
#elif defined( KINOMA_DCT_ARM )
	{
		IppStatus sts;
		Profile_Start(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
		sts = ippiDCT8x8Inv_4x4_16s8u_C1R_arm(pSrc, pDst, dstStep );
		Profile_End(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
		return sts;
	}
#endif
	return ippStsNoErr;
}



IppStatus __STDCALL  
ippiDCT8x8Inv_2x2_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
#if defined(KINOMA_DCT_C)
	Ipp16s Dst[64], *pTmp;
	Ipp8u *pD;
	int i;
	
	Profile_Start(ippiDCT8x8Inv_16s8u_C1R_c_profile);

	memcpy(Dst,pSrc,64*sizeof(Ipp16s));

	IDCT_ROW_2x2(Dst);
	IDCT_ROW_2x2(Dst+8);

	IDCT_COL_2x2(Dst);
	IDCT_COL_2x2(Dst+1);
	IDCT_COL_2x2(Dst+2);
	IDCT_COL_2x2(Dst+3);
	IDCT_COL_2x2(Dst+4);
	IDCT_COL_2x2(Dst+5);
	IDCT_COL_2x2(Dst+6);
	IDCT_COL_2x2(Dst+7);

	pTmp = Dst;
	pD = pDst;
	for (i = 0; i < 8; i++)
	{
		pD[0] = CLIP(pTmp[0]);
		pD[1] = CLIP(pTmp[1]);
		pD[2] = CLIP(pTmp[2]);
		pD[3] = CLIP(pTmp[3]);
		pD[4] = CLIP(pTmp[4]);
		pD[5] = CLIP(pTmp[5]);
		pD[6] = CLIP(pTmp[6]);
		pD[7] = CLIP(pTmp[7]);
		pD += dstStep;
		pTmp += 8;
	}
	
	Profile_End(ippiDCT8x8Inv_16s8u_C1R_c_profile);

#elif defined( INTEL_DCT )
	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s8u_C1R(pSrc, pDst, dstStep );
		return sts;
	}
#elif defined( KINOMA_DCT_ARM )
	{
		IppStatus sts;
		Profile_Start(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
		sts = ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v5(pSrc, pDst, dstStep );
		Profile_End(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
		return sts;
	}
#endif
	return ippStsNoErr;
}




IppStatus __STDCALL  
ippiDCT8x8Inv_DC_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
	#define mp4_Set8x8_8u(p, step, v)									\
	{																	\
		Ipp32u val;														\
																		\
		val = v + (v << 8);												\
		val += val << 16;												\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val; p += step;		\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val; p += step;		\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val; p += step;		\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val; p += step;		\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val; p += step;		\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val; p += step;		\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val; p += step;		\
		((Ipp32u*)p)[0] = val; ((Ipp32u*)p)[1] = val;					\
	}

	Ipp8u dc = (Ipp8u)((pSrc[0] + 4) >> 3);

	mp4_Set8x8_8u(pDst, dstStep, dc);

	return ippStsNoErr;
}


IppStatus __STDCALL  
ippiDCT8x8Inv_16s_C1I_c(Ipp16s* pSrcDst)
{
#if defined(KINOMA_DCT_C)
	Profile_Start(ippiDCT8x8Inv_16s_C1I_c_profile);

	IDCT_ROW(pSrcDst);
	IDCT_ROW(pSrcDst+8);
	IDCT_ROW(pSrcDst+16);
	IDCT_ROW(pSrcDst+24);
	IDCT_ROW(pSrcDst+32);
	IDCT_ROW(pSrcDst+40);
	IDCT_ROW(pSrcDst+48);
	IDCT_ROW(pSrcDst+56);

	IDCT_COL(pSrcDst);
	IDCT_COL(pSrcDst+1);
	IDCT_COL(pSrcDst+2);
	IDCT_COL(pSrcDst+3);
	IDCT_COL(pSrcDst+4);
	IDCT_COL(pSrcDst+5);
	IDCT_COL(pSrcDst+6);
	IDCT_COL(pSrcDst+7);

	Profile_End(ippiDCT8x8Inv_16s_C1I_c_profile);

#elif defined( INTEL_DCT )

	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s_C1I(pSrcDst);
		return sts;
	}
#elif defined( KINOMA_DCT_ARM )
	{
		IppStatus sts;
		Profile_Start(ippiDCT8x8Inv_16s_C1I_arm_profile);
		sts = ippiDCT8x8Inv_16s_C1I_arm(pSrcDst);
		Profile_End(ippiDCT8x8Inv_16s_C1I_arm_profile);
		return sts;
	}
#endif
	return ippStsNoErr;
}

IppStatus __STDCALL 
ippiDCT8x8Inv_4x4_16s_C1I_c(Ipp16s* pSrcDst)
{
#ifdef VM
	VM_IDCT(pSrcDst);
#elif defined( KINOMA_DCT_C )

	/* last  4 row is 0, need not to calc */
	IDCT_ROW_4x4(pSrcDst);
	IDCT_ROW_4x4(pSrcDst+8);
	IDCT_ROW_4x4(pSrcDst+16);
	IDCT_ROW_4x4(pSrcDst+24);

	IDCT_COL_4x4(pSrcDst);
	IDCT_COL_4x4(pSrcDst+1);
	IDCT_COL_4x4(pSrcDst+2);
	IDCT_COL_4x4(pSrcDst+3);
	IDCT_COL_4x4(pSrcDst+4);
	IDCT_COL_4x4(pSrcDst+5);
	IDCT_COL_4x4(pSrcDst+6);
	IDCT_COL_4x4(pSrcDst+7);
#elif defined( KINOMA_DCT_ARM )
	{
		IppStatus sts;
		Profile_Start(ippiDCT8x8Inv_4x4_16s_C1I_arm_profile);
		sts = ippiDCT8x8Inv_4x4_16s_C1I_arm(pSrcDst);
		Profile_End(ippiDCT8x8Inv_4x4_16s_C1I_arm_profile);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL  
ippiDCT8x8Inv_2x2_16s_C1I_c(Ipp16s* pSrcDst)
{
#ifdef VM
	VM_IDCT(pSrcDst);
#elif defined( KINOMA_DCT_C )

	/* last  6 row is 0, need not to calc */
	IDCT_ROW_2x2(pSrcDst);
	IDCT_ROW_2x2(pSrcDst+8);

	IDCT_COL_2x2(pSrcDst);
	IDCT_COL_2x2(pSrcDst+1);
	IDCT_COL_2x2(pSrcDst+2);
	IDCT_COL_2x2(pSrcDst+3);
	IDCT_COL_2x2(pSrcDst+4);
	IDCT_COL_2x2(pSrcDst+5);
	IDCT_COL_2x2(pSrcDst+6);
	IDCT_COL_2x2(pSrcDst+7);
#elif defined( KINOMA_DCT_ARM )
	{
		IppStatus sts;
		Profile_Start(ippiDCT8x8Inv_2x2_16s_C1I_arm_profile);
		sts = ippiDCT8x8Inv_2x2_16s_C1I_arm(pSrcDst);
		Profile_End(ippiDCT8x8Inv_2x2_16s_C1I_arm_profile);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL  
ippiDCT8x8Inv_DC_16s_C1I_c(Ipp16s* coeffMB)
{
	#define mp4_Set64_16s(val, pDst) \
	{ \
		int     i; \
		Ipp32u  v; \
		v = ((val) << 16) + (Ipp16u)(val); \
		for (i = 0; i < 32; i += 8) { \
			((Ipp32u*)(pDst))[i] = v; \
			((Ipp32u*)(pDst))[i+1] = v; \
			((Ipp32u*)(pDst))[i+2] = v; \
			((Ipp32u*)(pDst))[i+3] = v; \
			((Ipp32u*)(pDst))[i+4] = v; \
			((Ipp32u*)(pDst))[i+5] = v; \
			((Ipp32u*)(pDst))[i+6] = v; \
			((Ipp32u*)(pDst))[i+7] = v; \
		} \
	}
	
	Ipp16s dc = (Ipp16s)((coeffMB[0] + 4) >> 3);
	mp4_Set64_16s(dc, coeffMB);

	return ippStsNoErr;
}


#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM

#include "mp4.h"

IppStatus __STDCALL  
ippiDCT8x8Inv_16s8u_C1R_arm_v5(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
#if defined(KINOMA_DCT_C)||defined(KINOMA_DCT_ARM)
	__ALIGN16(Ipp16s, Dst, 64);
	int i;
	Ipp16s *pTmp;
	Ipp8u *pD;

	extern void ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5(Ipp16s *Dst, Ipp8u *pDst, int dstStep);
	
	//Profile_Start(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
	memcpy(Dst,pSrc,64*sizeof(Ipp16s));
	ippiDCT8x8Inv_16s_C1I_arm_v5(Dst);
	ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5(Dst, pDst, dstStep);

	//Profile_End(ippiDCT8x8Inv_16s8u_C1R_arm_profile);

#elif defined( INTEL_DCT )

	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s8u_C1R(pSrc, pDst, dstStep );
		return sts;
	}
#endif
	return ippStsNoErr;
}


IppStatus __STDCALL  
ippiDCT8x8Inv_4x4_16s8u_C1R_arm_v5(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
#if defined(KINOMA_DCT_C)||defined(KINOMA_DCT_ARM)
	__ALIGN16(Ipp16s, Dst, 64);
	int i;
	Ipp16s *pTmp;
	Ipp8u *pD;

	extern void ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5(Ipp16s *Dst, Ipp8u *pDst, int dstStep);
	
	//Profile_Start(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
	memcpy(Dst,pSrc,64*sizeof(Ipp16s));
	ippiDCT8x8Inv_4x4_16s_C1I_arm_v5(Dst);
	ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5(Dst, pDst, dstStep);
	//Profile_End(ippiDCT8x8Inv_16s8u_C1R_arm_profile);

#elif defined( INTEL_DCT )

	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s8u_C1R(pSrc, pDst, dstStep );
		return sts;
	}
#endif
	return ippStsNoErr;
}


IppStatus __STDCALL  
ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v5(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
#if defined(KINOMA_DCT_C)||defined(KINOMA_DCT_ARM)
	__ALIGN16(Ipp16s, Dst, 64);
	int i;
	Ipp16s *pTmp;
	Ipp8u *pD;

	extern void ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5(Ipp16s *Dst, Ipp8u *pDst, int dstStep);
	
	//Profile_Start(ippiDCT8x8Inv_16s8u_C1R_arm_profile);
	
	memcpy(Dst,pSrc,64*sizeof(Ipp16s));
	ippiDCT8x8Inv_2x2_16s_C1I_arm_v5(Dst);
	ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5(Dst, pDst, dstStep);
	//Profile_End(ippiDCT8x8Inv_16s8u_C1R_arm_profile);

#elif defined( INTEL_DCT )

	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s8u_C1R(pSrc, pDst, dstStep );
		return sts;
	}
#endif
	return ippStsNoErr;
}

#endif

IppStatus __STDCALL 
ippiDCT8x8Fwd_16s_C1R_c( const Ipp16s* pSrc, int srcStep, Ipp16s* pDst )
{
#if defined(KINOMA_DCT_C)
	int i, step = srcStep>>1;
	Ipp16s tmp[64], *pT;
	const Ipp16s *pS;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	pS = pSrc;
	pT = tmp;
	for (i = 0; i < 8; i++)
	{
		memcpy(pT,pS,sizeof(Ipp16s)*8);
		pS += step;
		pT += 8;
	}

	FDCT_ROW(tmp);
	FDCT_ROW(tmp+8);
	FDCT_ROW(tmp+16);
	FDCT_ROW(tmp+24);
	FDCT_ROW(tmp+32);
	FDCT_ROW(tmp+40);
	FDCT_ROW(tmp+48);
	FDCT_ROW(tmp+56);

	FDCT_COL(tmp);
	FDCT_COL(tmp+1);
	FDCT_COL(tmp+2);
	FDCT_COL(tmp+3);
	FDCT_COL(tmp+4);
	FDCT_COL(tmp+5);
	FDCT_COL(tmp+6);
	FDCT_COL(tmp+7);

	for (i = 0; i < 64; i++)
	{
		pDst[i] = (short int) DESCALE(tmp[i], 3);
	}

#elif defined( INTEL_DCT )

	{
		IppStatus sts;
		sts = ippiDCT8x8Fwd_16s_C1R(pSrc, srcStep, pDst );
		return sts;
	}
#endif
	return ippStsNoErr;
}

IppStatus __STDCALL  
ippiDCT8x8Fwd_8u16s_C1R_c( const Ipp8u* pSrc, int srcStep, Ipp16s* pDst)
{
#if defined(KINOMA_DCT_C)
	int i;
	Ipp16s tmp[64], *pT;
	const Ipp8u *pS;

	if (!pSrc || ! pDst)
		return ippStsNullPtrErr;

	pT = tmp;
	pS = pSrc;
	for (i = 0; i < 8; i++)
	{
		pT[0] = pS[0];
		pT[1] = pS[1];
		pT[2] = pS[2];
		pT[3] = pS[3];
		pT[4] = pS[4];
		pT[5] = pS[5];
		pT[6] = pS[6];
		pT[7] = pS[7];
		pS += srcStep;
		pT += 8;
	}

	FDCT_ROW(tmp);
	FDCT_ROW(tmp+8);
	FDCT_ROW(tmp+16);
	FDCT_ROW(tmp+24);
	FDCT_ROW(tmp+32);
	FDCT_ROW(tmp+40);
	FDCT_ROW(tmp+48);
	FDCT_ROW(tmp+56);

	FDCT_COL(tmp);
	FDCT_COL(tmp+1);
	FDCT_COL(tmp+2);
	FDCT_COL(tmp+3);
	FDCT_COL(tmp+4);
	FDCT_COL(tmp+5);
	FDCT_COL(tmp+6);
	FDCT_COL(tmp+7);

	for (i = 0; i < 64; i++)
	{
		pDst[i] = (short int) DESCALE(tmp[i], 3);
	}

#elif defined( INTEL_DCT )

	{
		IppStatus sts;
		sts = ippiDCT8x8Fwd_8u16s_C1R(pSrc, srcStep,  pDst);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL  
ippiDCT8x8Fwd_16s_C1I_c(Ipp16s* pSrcDst)
{
#if defined(KINOMA_DCT_C)
	int i;
	if (!pSrcDst)
		return ippStsNullPtrErr;

	FDCT_ROW(pSrcDst);
	FDCT_ROW(pSrcDst+8);
	FDCT_ROW(pSrcDst+16);
	FDCT_ROW(pSrcDst+24);
	FDCT_ROW(pSrcDst+32);
	FDCT_ROW(pSrcDst+40);
	FDCT_ROW(pSrcDst+48);
	FDCT_ROW(pSrcDst+56);

	FDCT_COL(pSrcDst);
	FDCT_COL(pSrcDst+1);
	FDCT_COL(pSrcDst+2);
	FDCT_COL(pSrcDst+3);
	FDCT_COL(pSrcDst+4);
	FDCT_COL(pSrcDst+5);
	FDCT_COL(pSrcDst+6);
	FDCT_COL(pSrcDst+7);

	for (i = 0; i < 64; i++)
	{
		pSrcDst[i] = (short int) DESCALE(pSrcDst[i], 3);
	}

#elif defined( INTEL_DCT )

	{
		IppStatus sts;
		sts = ippiDCT8x8Fwd_16s_C1I(pSrcDst);
		return sts;
	}
#elif defined( KINOMA_DCT_ARM )

	{
		IppStatus sts;
		sts = ippiDCT8x8Inv_16s_C1I_arm(pSrcDst);
		return sts;
	}

#endif

	return ippStsNoErr;
}

