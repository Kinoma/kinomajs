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
#ifndef INTEL_IDCT
#include "kinoma_ipp_lib.h"

#include "ippi.h"
#include "kinoma_mpeg4_assistant.h"

/*--------------------------------------------look here firstly---------------/ 
	two method: 
	1,VM_IDCT is algorithm modified from VM

	2,other is modified from "MPEG Software Simulation Group", 
	  And its declaration is 
	 "These software programs are available to the user without 
	 any license fee or * royalty on an "as is" basis."

	 the current test for one CIF bitstream, the result of this fast algorithm 
	 is more closed to VM's result than Intel's IDCT
-----------------------------------------------thank you---------------------*/

#ifdef VM

/************************** VM algorithm **************************/

static double c0=0.7071068;
static double c1=0.4903926;
static double c2=0.4619398;
static double c3=0.4157348;
static double c4=0.3535534;
static double c5=0.2777851;
static double c6=0.1913417;
static double c7=0.0975452; 

void VM_IDCT(Ipp16s *pSrcDst)
{
	int    j1, i, j;
	double tmp[8], tmp1[8];
	double e, f, g, h,block[8][8];
	int v, pos;
	Ipp16u *pD;

	/* Horizontal */

	/* Descan coefficients first */
	for (i = 0; i < 8; i++) 
	{
		pos = i << 3; /* i * 8 */
		for (j = 0; j < 8; j++) 
		{
			tmp[j] =(double)pSrcDst[pos+j]; 
		}
		e = tmp[1] * c7 - tmp[7] * c1;
		h = tmp[7] * c7 + tmp[1] * c1;
		f = tmp[5] * c3 - tmp[3] * c5;
		g = tmp[3] * c3 + tmp[5] * c5;

		tmp1[0] = (tmp[0] + tmp[4]) * c4;
		tmp1[1] = (tmp[0] - tmp[4]) * c4;
		tmp1[2] = tmp[2] * c6 - tmp[6] * c2;
		tmp1[3] = tmp[6] * c6 + tmp[2] * c2;
		tmp[4] = e + f;
		tmp1[5] = e - f;
		tmp1[6] = h - g;
		tmp[7] = h + g;

		tmp[5] = (tmp1[6] - tmp1[5]) * c0;
		tmp[6] = (tmp1[6] + tmp1[5]) * c0;
		tmp[0] = tmp1[0] + tmp1[3];
		tmp[1] = tmp1[1] + tmp1[2];
		tmp[2] = tmp1[1] - tmp1[2];
		tmp[3] = tmp1[0] - tmp1[3];

		for (j = 0; j < 4; j++) 
		{
			j1 = 7 - j;
			block[i][j] = tmp[j] + tmp[j1];
			block[i][j1] = tmp[j] - tmp[j1];
		}
	}

	/* Vertical */
	for (i = 0; i < 8; i++) 
	{
		for (j = 0; j < 8; j++) 
		{
			tmp[j] = block[j][i];
		}
		e = tmp[1] * c7 - tmp[7] * c1;
		h = tmp[7] * c7 + tmp[1] * c1;
		f = tmp[5] * c3 - tmp[3] * c5;
		g = tmp[3] * c3 + tmp[5] * c5;

		tmp1[0] = (tmp[0] + tmp[4]) * c4;
		tmp1[1] = (tmp[0] - tmp[4]) * c4;
		tmp1[2] = tmp[2] * c6 - tmp[6] * c2;
		tmp1[3] = tmp[6] * c6 + tmp[2] * c2;
		tmp[4] = e + f;
		tmp1[5] = e - f;
		tmp1[6] = h - g;
		tmp[7] = h + g;

		tmp[5] = (tmp1[6] - tmp1[5]) * c0;
		tmp[6] = (tmp1[6] + tmp1[5]) * c0;
		tmp[0] = tmp1[0] + tmp1[3];
		tmp[1] = tmp1[1] + tmp1[2];
		tmp[2] = tmp1[1] - tmp1[2];
		tmp[3] = tmp1[0] - tmp1[3];

		for (j = 0; j < 4; j++) 
		{
			j1 = 7 - j;
			block[j][i] = tmp[j] + tmp[j1];
			block[j1][i] = tmp[j] - tmp[j1];
		}
	}    

	pD = pSrcDst;
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			v = (int) floor (block[i][j] + 0.5);
			pD[j] = CLIP_R(v,255,-256);
		}
		pD += 8;
	}
}

#else		/* end of VM func */

/*-------------------- VM algorithm end------------------------------*/



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
static void __inline IDCT_ROW (short *pSrcDst)
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
static void __inline IDCT_COL(short *pSrcDst)
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

#endif /* end of #ifdef VM  #else */




IppStatus __STDCALL  
ippiDCT8x8Inv_16s8u_C1R_c(const Ipp16s* pSrc, Ipp8u* pDst, int dstStep)
{
#ifdef VM
	int    j1, i, j;
	double tmp[8], tmp1[8];
	double e, f, g, h,block[8][8];
	int v, pos;
	Ipp8u *pD;

	for (i = 0; i < 8; i++) 
	{
		pos = i << 3; /* i*8  */
		for (j = 0; j < 8; j++) 
		{
			tmp[j] =(double)pSrc[pos+j]; 
		}
		e = tmp[1] * c7 - tmp[7] * c1;
		h = tmp[7] * c7 + tmp[1] * c1;
		f = tmp[5] * c3 - tmp[3] * c5;
		g = tmp[3] * c3 + tmp[5] * c5;

		tmp1[0] = (tmp[0] + tmp[4]) * c4;
		tmp1[1] = (tmp[0] - tmp[4]) * c4;
		tmp1[2] = tmp[2] * c6 - tmp[6] * c2;
		tmp1[3] = tmp[6] * c6 + tmp[2] * c2;
		tmp[4] = e + f;
		tmp1[5] = e - f;
		tmp1[6] = h - g;
		tmp[7] = h + g;

		tmp[5] = (tmp1[6] - tmp1[5]) * c0;
		tmp[6] = (tmp1[6] + tmp1[5]) * c0;
		tmp[0] = tmp1[0] + tmp1[3];
		tmp[1] = tmp1[1] + tmp1[2];
		tmp[2] = tmp1[1] - tmp1[2];
		tmp[3] = tmp1[0] - tmp1[3];

		for (j = 0; j < 4; j++) 
		{
			j1 = 7 - j;
			block[i][j] = tmp[j] + tmp[j1];
			block[i][j1] = tmp[j] - tmp[j1];
		}
	}

	/* Vertical */
	for (i = 0; i < 8; i++) 
	{
		for (j = 0; j < 8; j++) 
		{
			tmp[j] = block[j][i];
		}
		e = tmp[1] * c7 - tmp[7] * c1;
		h = tmp[7] * c7 + tmp[1] * c1;
		f = tmp[5] * c3 - tmp[3] * c5;
		g = tmp[3] * c3 + tmp[5] * c5;

		tmp1[0] = (tmp[0] + tmp[4]) * c4;
		tmp1[1] = (tmp[0] - tmp[4]) * c4;
		tmp1[2] = tmp[2] * c6 - tmp[6] * c2;
		tmp1[3] = tmp[6] * c6 + tmp[2] * c2;
		tmp[4] = e + f;
		tmp1[5] = e - f;
		tmp1[6] = h - g;
		tmp[7] = h + g;

		tmp[5] = (tmp1[6] - tmp1[5]) * c0;
		tmp[6] = (tmp1[6] + tmp1[5]) * c0;
		tmp[0] = tmp1[0] + tmp1[3];
		tmp[1] = tmp1[1] + tmp1[2];
		tmp[2] = tmp1[1] - tmp1[2];
		tmp[3] = tmp1[0] - tmp1[3];

		for (j = 0; j < 4; j++) 
		{
			j1 = 7 - j;
			block[j][i] = tmp[j] + tmp[j1];
			block[j1][i] = tmp[j] - tmp[j1];
		}
	}    

	pD = pDst;
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			v = (int) floor (block[i][j] + 0.5);
			pD[j] = CLIP(v);
		}
		pD += dstStep;
	}
#else
	int i;
	Ipp16s Dst[64], *pTmp;
	Ipp8u *pD;
	
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

#endif

	return ippStsNoErr;
}

IppStatus __STDCALL  
ippiDCT8x8Inv_16s_C1I_c(Ipp16s* pSrcDst)
{
#ifdef VM
	VM_IDCT(pSrcDst);
#else
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
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL 
ippiDCT8x8Inv_4x4_16s_C1I_c(Ipp16s* pSrcDst)
{
#ifdef VM
	VM_IDCT(pSrcDst);
#else
	/* last  4 row is 0, need not to calc */
	IDCT_ROW(pSrcDst);
	IDCT_ROW(pSrcDst+8);
	IDCT_ROW(pSrcDst+16);
	IDCT_ROW(pSrcDst+24);

	IDCT_COL(pSrcDst);
	IDCT_COL(pSrcDst+1);
	IDCT_COL(pSrcDst+2);
	IDCT_COL(pSrcDst+3);
	IDCT_COL(pSrcDst+4);
	IDCT_COL(pSrcDst+5);
	IDCT_COL(pSrcDst+6);
	IDCT_COL(pSrcDst+7);
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL  
ippiDCT8x8Inv_2x2_16s_C1I_c(Ipp16s* pSrcDst)
{
#ifdef VM
	VM_IDCT(pSrcDst);
#else
	/* last  6 row is 0, need not to calc */
	IDCT_ROW(pSrcDst);
	IDCT_ROW(pSrcDst+8);

	IDCT_COL(pSrcDst);
	IDCT_COL(pSrcDst+1);
	IDCT_COL(pSrcDst+2);
	IDCT_COL(pSrcDst+3);
	IDCT_COL(pSrcDst+4);
	IDCT_COL(pSrcDst+5);
	IDCT_COL(pSrcDst+6);
	IDCT_COL(pSrcDst+7);
#endif

	return ippStsNoErr;
}

#endif
