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
#include "Fsk.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"

IppStatus (__STDCALL *ippiCopy8x8QP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc,int srcStep,Ipp8u* pDst,int dstStep,int acc,int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy8x4HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy8x8HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x8HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x8QP_MPEG4_8u_C1R_universal)			(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x16HP_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiCopy16x16QP_MPEG4_8u_C1R_universal)			(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep,int acc, int rounding)=NULL;

/* for quarter pel calc */

/*#define FILTER(tmp1,tmp2,tmp3,tmp4,round) \
		( (((tmp1)<<7) + ((tmp1)<<5)) - (((tmp2)<<5) + ((tmp2)<<4)) + (((tmp3)<<4) + ((tmp3)<<3)) - ((tmp4) << 3) + (round)) >> 8
*/

/*#define FILTER(T1,T2,T3,T4,R) \
		( ((T1)<<7) + (((T1)-(T2))<<5) + (((T3)-(T2))<<4) + (((T3)-(T4))<< 3) + (R)) >> 8
*/

/*#define FILTER(T1,T2,T3,T4,R) \
		(  (  ((((T1)<<4) + ((T1)<<2)) - (((T2)<<2) + ((T2)<<1)) + (((T3)<<1) + (T3)) - (T4) )<<3) + (R) ) >> 8
*/

/*#define FILTER(T1,T2,T3,T4,R) \
		(  ((((T1)<<4) + (((T1)-(T2))<<2) + (((T3)-(T2))<<1) + (T3)-(T4))<<3) + (R) ) >> 8
*/

#define FILTER(T1,T2,T3,T4,R) \
		(  (T1)*160 - (T2)* 48 +(T3) *24 - (T4) * 8 + (R) ) >> 8

/*#define FILTER(T1,T2,T3,T4,R) \
		(  (((T1)*20 - (T2)* 6 +(T3) *3 - (T4)) * 8) + (R) ) >> 8
*/



/*-----------------------------------------------------------------------------
*		quarter pixel interpolation about
*----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Calculation of the quarter sample values
figure 7-32 iso-14496-2:2004(e) page: 342
(A)+   (e)x   (b)o   (f)x  (A)+

(g)x   (h)x   (i)x   (j)x

(c)o   (k)x   (d)o   (l)x

(m)x   (n)x   (o)x   (p)x

(A)+   (e)x   (b)o   (f)x  (A)+    

+: Integer sample position; o: Half sample position; x: Quarter sample position
-----------------------------------------------------------------------------*/

static int QP_CLIP[]=
{
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
 20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
 44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,
 56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,
 68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,
 92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103,
104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211,
212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235,
236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
248, 249, 250, 251, 252, 253, 254, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};
static int * pQPClip = QP_CLIP+112;


void calc_QP_only_h(const unsigned char *pSrc,int srcStep,int srcWidth,int srcHeight,
			unsigned char *pDst, int dstStep,int dstWidth, int dstHeight, int xFrac, int rounding)
{
 	int i,j;
	Ipp8u pSrcBlock[384];	/* 24 * 16 */
	Ipp8u *pSB;
	Ipp8u *pD, *pS;
	const int mirWidth = 24;

	register int tmp;
	int round, round1 = 1 - rounding;
	
#ifdef WIN32
	unsigned int* p1 ;
	unsigned int* p2 ;
#endif

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;
  	for (j = 0; j < dstHeight; j++)
	{
#if defined(WIN32)&&!defined(_WIN32_WCE)
		p1 = (unsigned int*)pSB;
		p2 = (unsigned int*)pS;
		for (i = 0; i < dstWidth; i+=4)
		{
			*(p1++) = *(p2++);
		}
		/* pSB[dstWidth] = pS[dstWidth];  move to mirror  loop */
#else
		/*for (i = 0; i < srcWidth; i++)
		{
			pSB[i] = pS[i];
		}*/
		memcpy(pSB,pS,srcWidth);
#endif
		/* do not use this, time consuming 
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
		pSB[dstWidth+1] = pS[srcWidth-1];
		*/
		pS += srcStep;
		pSB += mirWidth;
	}

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;
	for (j = 0; j < dstHeight; j++)
	{
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
#ifdef WIN32
		pSB[dstWidth+1] = pSB[dstWidth] = pS[dstWidth];
#else 
		pSB[dstWidth+1] = pS[dstWidth];
#endif 
		pS += srcStep;
		pSB += mirWidth;
	}

	round = 128 - rounding;

	pSB = pSrcBlock + 3;
	pD = pDst;
	switch(xFrac)
	{
	case 1:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pD[i] = (pSB[i] + pQPClip[tmp] + round1)>>1;
				/*pD[i] = (pSB[i] + CLIP(tmp) + round1)>>1;*/
			}
			pD += dstStep;
			pSB += mirWidth;
		}
		break;

	case 2:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pD[i] = pQPClip[tmp];
				/*pD[i] =  CLIP(tmp);*/
			}
			pD += dstStep;
			pSB += mirWidth;
		}
		break;

	case 3:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pD[i] = (pSB[i+1] + pQPClip[tmp] + round1)>>1;
				/*pD[i] = (pSB[i+1] + CLIP(tmp) + round1)>>1;*/
			}
			pD += dstStep;
			pSB += mirWidth;
		}
		break;
	default:
		break;
	}

}


void calc_QP_only_v(const unsigned char *pSrc,int srcStep,int srcWidth,int srcHeight,
			unsigned char *pDst, int dstStep,int dstWidth, int dstHeight, int yFrac, int rounding)
{
	int i,j;
	Ipp8u pTmpBlock[368];	/*16 * 23*/
	Ipp8u *pTmp,*pD, *pS;
	int mirHeight;
	const int memWidth = 16;
	const int dstWidth2 = 32;
	const int dstWidth3 = 48;
	const int dstWidth4 = 64;

	register int tmp;
	int round,round1 = 1- rounding;
	
#ifdef WIN32
	unsigned int* p1 ;
	unsigned int* p2 ;
#endif
	pD = pDst;
	pS = (Ipp8u *)pSrc;

	mirHeight = srcHeight + 6;

	pTmp = pTmpBlock + dstWidth3;

  	for (j = 0; j < srcHeight; j++)
	{
#if defined(WIN32)&&!defined(_WIN32_WCE)
		p1 = (unsigned int*)pTmp;
		p2 = (unsigned int*)pS;
		for (i = 0; i < dstWidth; i+=4)
		{
			*(p1++) = *(p2++);
		}
#else
		/*
		for (i = 0; i < dstWidth; i++)
		{
			pTmp[i] = pS[i];
		}*/
		memcpy(pTmp, pS,dstWidth);
#endif
		pTmp += memWidth;
		pS += srcStep;
	}
	round = 128 - rounding;

	pTmp = pTmpBlock + dstWidth3;

	/*memcpy(pTmp-memWidth, pTmp, dstWidth);
	memcpy(pTmp-dstWidth2, pTmp+memWidth, dstWidth);
	memcpy(pTmp-dstWidth3, pTmp+dstWidth2, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i-memWidth] = pTmp[i];
		pTmp[i-dstWidth2] = pTmp[i+memWidth];
		pTmp[i-dstWidth3] = pTmp[i+dstWidth2];
	}

	pTmp = pTmpBlock + (memWidth * (mirHeight-3));
	
	/*memcpy(pTmp, pTmp-memWidth, dstWidth);
	memcpy(pTmp+memWidth, pTmp-dstWidth2, dstWidth);
	memcpy(pTmp+dstWidth2,pTmp-dstWidth3, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i] = pTmp[i-memWidth];
		pTmp[i+memWidth] = pTmp[i-dstWidth2];
		pTmp[i+dstWidth2] = pTmp[i-dstWidth3];
	}


	pTmp = pTmpBlock + dstWidth3;

	switch(yFrac)
	{
	case 1:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;
	case 2:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] =  pQPClip[tmp];
				/*pD[i] =  CLIP(tmp);*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;
	case 3:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i+memWidth] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i+memWidth] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;
	default:
		break;
	}
}


void calc_QP_bias(const Ipp8u *pSrc, int srcStep, int srcWidth, int srcHeight,
							Ipp8u *pDst,int dstStep,int dstWidth, int dstHeight,
							int xFrac,int yFrac, int rounding)
{
	register int tmp;
 	int i,j,round,round1 = 1 - rounding;
	Ipp8u *pTmp, *pSB, *pD, *pS;
	Ipp8u pSrcBlock[408],pTmpBlock[368];	/* 24 * 17, 16 * 23 */
	const int mirWidth = 24;
	const int memWidth = 16;
	const int dstWidth2 = 32;
	const int dstWidth3 = 48;
	const int dstWidth4 = 64;
	int mirHeight = srcHeight + 6;

#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;

  	for (j = 0; j < srcHeight; j++)
	{
#if defined(WIN32)&&!defined(_WIN32_WCE)
		p1 = (unsigned int*)pSB;
		p2 = (unsigned int*)pS;
		for (i = 0; i < dstWidth; i+=4)
		{
			*(p1++) = *(p2++);
		}
		/* pSB[dstWidth] = pS[dstWidth];  move to mirror loop */
#else
		/*for (i = 0; i < srcWidth; i++)
		  {
			pSB[i] = pS[i];
		  }
		  */
		memcpy(pSB,pS,srcWidth);
#endif
		/* do not use this, time consuming 
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
		pSB[dstWidth+1] = pS[srcWidth-1];
		*/
		pS += srcStep;
		pSB += mirWidth;
	}

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;

	for (j = 0; j < srcHeight; j++)
	{
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
#ifdef WIN32
		pSB[dstWidth+1] = pSB[dstWidth] = pS[dstWidth];
#else 
		pSB[dstWidth+1] = pS[dstWidth];
#endif 
		pS += srcStep;
		pSB += mirWidth;

	}

	round = 128 - rounding;

	pTmp = pTmpBlock + dstWidth3;
	pSB = pSrcBlock + 3;
	switch(xFrac)
	{
	case 1:
		for (j = 0; j < srcHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pTmp[i] = (pSB[i] + pQPClip[tmp] + round1) >>1;
				/*pTmp[i] = (pSB[i] + CLIP(tmp) + round1) >>1;*/
			}
			pTmp += memWidth;
			pSB += mirWidth;

		}
		break;

	case 2:
		for (j = 0; j < srcHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pTmp[i] =  pQPClip[tmp];
				/*pTmp[i] =  CLIP(tmp);*/
			}
			pTmp += memWidth;
			pSB += mirWidth;
		}
		break;

	case 3:
		for (j = 0; j < srcHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pTmp[i] = (pSB[i+1] + pQPClip[tmp] + round1)>>1;
				/*pTmp[i] = (pSB[i+1] + CLIP(tmp) + round1)>>1;*/
			}
			pTmp += memWidth;
			pSB += mirWidth;
		}
		break;

	default:
		break;
	}


	pTmp = pTmpBlock + dstWidth3;
	/*memcpy(pTmp-memWidth, pTmp, dstWidth);
	memcpy(pTmp-dstWidth2, pTmp+memWidth, dstWidth);
	memcpy(pTmp-dstWidth3, pTmp+dstWidth2, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i-memWidth] = pTmp[i];
		pTmp[i-dstWidth2] = pTmp[i+memWidth];
		pTmp[i-dstWidth3] = pTmp[i+dstWidth2];
	}

	pTmp = pTmpBlock + (memWidth * (mirHeight-3));
	/*memcpy(pTmp, pTmp-memWidth, dstWidth);
	memcpy(pTmp+memWidth, pTmp-dstWidth2, dstWidth);
	memcpy(pTmp+dstWidth2,pTmp-dstWidth3, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i] = pTmp[i-memWidth];
		pTmp[i+memWidth] = pTmp[i-dstWidth2];
		pTmp[i+dstWidth2] = pTmp[i-dstWidth3];
	}

	pTmp = pTmpBlock + dstWidth3;
	pD = pDst;
	switch(yFrac)
	{
	case 1:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;

	case 2:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] =  pQPClip[tmp];
				/*pD[i] =  CLIP(tmp);*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;

	case 3:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i+memWidth] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i+memWidth] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;

	default:
		break;
	}

}


IppStatus __STDCALL 
ippiCopy8x4HP_8u_C1R_c(
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding)
{
	int i;
	register int round;
	Ipp8u *pS, *pD;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	round = 1 - rounding;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy8x4HP_8u_C1R		with acc(0,1,2,3): ",acc);
		dump("ippiCopy8x4HP_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	pS = (Ipp8u*)pSrc;
	pD = pDst;

	if (acc == 0)		/*the a points, every point need not to think the border of block*/
	{
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (i = 4; i > 0; i--)
		{
#if defined(WIN32)&&!defined(_WIN32_WCE)
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*(p1++) = *(p2++);
			*p1 = *p2;
#else
			/*
			pD[0] = pS[0];
			pD[1] = pS[1];
			pD[2] = pS[2];
			pD[3] = pS[3];
			pD[4] = pS[4];
			pD[5] = pS[5];
			pD[6] = pS[6];
			pD[7] = pS[7];
			*/
			memcpy(pD,pS,8);
#endif
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 1)		/* the b points*/
	{
		for (i = 4; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + round)>>1;
			pD[1] = (pS[1] + pS[2] + round)>>1;
			pD[2] = (pS[2] + pS[3] + round)>>1;
			pD[3] = (pS[3] + pS[4] + round)>>1;
			pD[4] = (pS[4] + pS[5] + round)>>1;
			pD[5] = (pS[5] + pS[6] + round)>>1;
			pD[6] = (pS[6] + pS[7] + round)>>1;
			pD[7] = (pS[7] + pS[8] + round)>>1;
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 2)		/* the c points*/
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;
		for (i = 4; i > 0; i--)
		{
			pD[0] = (pS[0] + pSN[0] + round)>>1;
			pD[1] = (pS[1] + pSN[1] + round)>>1;
			pD[2] = (pS[2] + pSN[2] + round)>>1;
			pD[3] = (pS[3] + pSN[3] + round)>>1;
			pD[4] = (pS[4] + pSN[4] + round)>>1;
			pD[5] = (pS[5] + pSN[5] + round)>>1;
			pD[6] = (pS[6] + pSN[6] + round)>>1;
			pD[7] = (pS[7] + pSN[7] + round)>>1;

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 3)		/* the d points */
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;
		int round = 2 - rounding;
		for (i = 4; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + pSN[0] + pSN[1] + round)>>2;
			pD[1] = (pS[1] + pS[2] + pSN[1] + pSN[2] + round)>>2;
			pD[2] = (pS[2] + pS[3] + pSN[2] + pSN[3] + round)>>2;
			pD[3] = (pS[3] + pS[4] + pSN[3] + pSN[4] + round)>>2;
			pD[4] = (pS[4] + pS[5] + pSN[4] + pSN[5] + round)>>2;
			pD[5] = (pS[5] + pS[6] + pSN[5] + pSN[6] + round)>>2;
			pD[6] = (pS[6] + pS[7] + pSN[6] + pSN[7] + round)>>2;
			pD[7] = (pS[7] + pS[8] + pSN[7] + pSN[8] + round)>>2;

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}	
	}

	return ippStsNoErr;
}


/*
	source is misaligned ~25% of the time
	destination is never misaligned
	distbution of acc is content dependent - roughly flat to skewed towards aac=3 (high motion)
*/

IppStatus __STDCALL 
ippiCopy8x8HP_8u_C1R_c(							
						const Ipp8u* pSrc,
						int          srcStep,
						Ipp8u*       pDst,
						int          dstStep,
						int          acc,
						int          rounding)
{
	int i;
	Ipp8u *pS, *pD;

#if _DEBUG
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
#endif

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy8x8HP_8u_C1R		with acc(0,1,2,3): ",acc);
		dump("ippiCopy8x8HP_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	pS = (Ipp8u*)pSrc;
	pD = pDst;

	if (acc == 0)		/*the a points, every point need not to think the border of block*/
	{
		if (3 & (int)pS)
		{
			// misaligned source
			for (i = 8; i > 0; i--)
			{
				pD[0] = pS[0];
				pD[1] = pS[1];
				pD[2] = pS[2];
				pD[3] = pS[3];
				pD[4] = pS[4];
				pD[5] = pS[5];
				pD[6] = pS[6];
				pD[7] = pS[7];

				pD += dstStep;
				pS += srcStep;
			}
		}
		else
		{
			// aligned source
			for (i = 8; i > 0; i--)
			{
				*(int *)(pD + 0) = *(int *)(pS + 0);
				*(int *)(pD + 4) = *(int *)(pS + 4);

				pD += dstStep;
				pS += srcStep;
			}
		}
	}
	else if (acc == 1)		/* the b points*/
	{
		int round = 1 - rounding;

		for (i = 8; i > 0; i--)
		{
#if 0
			pD[0] = (pS[0] + pS[1] + round)>>1;
			pD[1] = (pS[1] + pS[2] + round)>>1;
			pD[2] = (pS[2] + pS[3] + round)>>1;
			pD[3] = (pS[3] + pS[4] + round)>>1;
			pD[4] = (pS[4] + pS[5] + round)>>1;
			pD[5] = (pS[5] + pS[6] + round)>>1;
			pD[6] = (pS[6] + pS[7] + round)>>1;
			pD[7] = (pS[7] + pS[8] + round)>>1;
#else
			int s0, s1;

			s0 = pS[1] + round;
			pD[0] = (pS[0] + s0) >> 1;
			s1 = pS[2];
			pD[1] = (s0 + s1) >> 1;
			s0 = pS[3] + round;
			pD[2] = (s0 + s1) >> 1;
			s1 = pS[4];
			pD[3] = (s0 + s1) >> 1;
			s0 = pS[5] + round;
			pD[4] = (s0 + s1) >> 1;
			s1 = pS[6];
			pD[5] = (s0 + s1) >> 1;
			s0 = pS[7] + round;
			pD[6] = (s0 + s1) >> 1;
			pD[7] = (s0 + pS[8]) >> 1;
#endif
			pS += srcStep;
			pD += dstStep;
		}

	}
	else if (acc == 2)		/* the c points*/
	{
		int round = 1 - rounding;
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;

		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pSN[0] + round)>>1;
			pD[1] = (pS[1] + pSN[1] + round)>>1;
			pD[2] = (pS[2] + pSN[2] + round)>>1;
			pD[3] = (pS[3] + pSN[3] + round)>>1;
			pD[4] = (pS[4] + pSN[4] + round)>>1;
			pD[5] = (pS[5] + pSN[5] + round)>>1;
			pD[6] = (pS[6] + pSN[6] + round)>>1;
			pD[7] = (pS[7] + pSN[7] + round)>>1;

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}

	}
	else if (acc == 3)		/* the d points */
	{
		int round = 2 - rounding;
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;
		for (i = 8; i > 0; i--)
		{
#if 0
			pD[0] = (pS[0] + pS[1] + pSN[0] + pSN[1] + round)>>2;
			pD[1] = (pS[1] + pS[2] + pSN[1] + pSN[2] + round)>>2;
			pD[2] = (pS[2] + pS[3] + pSN[2] + pSN[3] + round)>>2;
			pD[3] = (pS[3] + pS[4] + pSN[3] + pSN[4] + round)>>2;
			pD[4] = (pS[4] + pS[5] + pSN[4] + pSN[5] + round)>>2;
			pD[5] = (pS[5] + pS[6] + pSN[5] + pSN[6] + round)>>2;
			pD[6] = (pS[6] + pS[7] + pSN[6] + pSN[7] + round)>>2;
			pD[7] = (pS[7] + pS[8] + pSN[7] + pSN[8] + round)>>2;
#else
			int s0, s1;

			s0 = pS[1] + pSN[1] + round;
			pD[0] = (pS[0] + pSN[0] + s0) >> 2;
			s1 = pS[2] + pSN[2];
			pD[1] = (s0 + s1) >> 2;
			s0 = pS[3] + pSN[3] + round;
			pD[2] = (s0 + s1) >> 2;
			s1 = pS[4] + pSN[4];
			pD[3] = (s0 + s1) >> 2;
			s0 = pS[5] + pSN[5] + round;
			pD[4] = (s0 + s1) >> 2;
			s1 = pS[6] + pSN[6];
			pD[5] = (s0 + s1) >> 2;
			s0 = pS[7] + pSN[7] + round;
			pD[6] = (s0 + s1) >> 2;
			pD[7] = (s0 + pS[8] + pSN[8]) >> 2;
#endif
			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}	
	}

	return ippStsNoErr;
}

IppStatus __STDCALL 
ippiCopy16x8HP_8u_C1R_c(
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding)
{
	int i;
	register int round;
	Ipp8u *pS, *pD;
	
	round = 1 - rounding;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy16x8HP_8u_C1R		with acc(0,1,2,3): ",acc);
		dump("ippiCopy16x8HP_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	pS = (Ipp8u*)pSrc;
	pD = pDst;

	if (acc == 0)
	{
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (i = 8; i > 0; i--)
		{
#if defined(WIN32)&&!defined(_WIN32_WCE)
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*p1 = *p2;
#else
			/*
			pD[0] = pS[0];
			pD[1] = pS[1];
			pD[2] = pS[2];
			pD[3] = pS[3];
			pD[4] = pS[4];
			pD[5] = pS[5];
			pD[6] = pS[6];
			pD[7] = pS[7];
			pD[8] = pS[8];
			pD[9] = pS[9];
			pD[10] = pS[10];
			pD[11] = pS[11];
			pD[12] = pS[12];
			pD[13] = pS[13];
			pD[14] = pS[14];
			pD[15] = pS[15];
			*/
			memcpy(pD,pS,16);
#endif
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 1)
	{
		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + round)>>1;
			pD[1] = (pS[1] + pS[2] + round)>>1;
			pD[2] = (pS[2] + pS[3] + round)>>1;
			pD[3] = (pS[3] + pS[4] + round)>>1;
			pD[4] = (pS[4] + pS[5] + round)>>1;
			pD[5] = (pS[5] + pS[6] + round)>>1;
			pD[6] = (pS[6] + pS[7] + round)>>1;
			pD[7] = (pS[7] + pS[8] + round)>>1;
			pD[8] = (pS[8] + pS[9] + round)>>1;
			pD[9] = (pS[9] + pS[10] + round)>>1;
			pD[10] = (pS[10] + pS[11] + round)>>1;
			pD[11] = (pS[11] + pS[12] + round)>>1;
			pD[12] = (pS[12] + pS[13] + round)>>1;
			pD[13] = (pS[13] + pS[14] + round)>>1;
			pD[14] = (pS[14] + pS[15] + round)>>1;
			pD[15] = (pS[15] + pS[16] + round)>>1;
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 2)
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;

		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pSN[0] + round)>>1;
			pD[1] = (pS[1] + pSN[1] + round)>>1;
			pD[2] = (pS[2] + pSN[2] + round)>>1;
			pD[3] = (pS[3] + pSN[3] + round)>>1;
			pD[4] = (pS[4] + pSN[4] + round)>>1;
			pD[5] = (pS[5] + pSN[5] + round)>>1;
			pD[6] = (pS[6] + pSN[6] + round)>>1;
			pD[7] = (pS[7] + pSN[7] + round)>>1;
			pD[8] = (pS[8] + pSN[8] + round)>>1;
			pD[9] = (pS[9] + pSN[9] + round)>>1;
			pD[10] = (pS[10] + pSN[10] + round)>>1;
			pD[11] = (pS[11] + pSN[11] + round)>>1;
			pD[12] = (pS[12] + pSN[12] + round)>>1;
			pD[13] = (pS[13] + pSN[13] + round)>>1;
			pD[14] = (pS[14] + pSN[14] + round)>>1;
			pD[15] = (pS[15] + pSN[15] + round)>>1;			
			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}

	}
	else if (acc == 3)
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;
		round = 2 - rounding;
		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + pSN[0] + pSN[1] + round)>>2;
			pD[1] = (pS[1] + pS[2] + pSN[1] + pSN[2] + round)>>2;
			pD[2] = (pS[2] + pS[3] + pSN[2] + pSN[3] + round)>>2;
			pD[3] = (pS[3] + pS[4] + pSN[3] + pSN[4] + round)>>2;
			pD[4] = (pS[4] + pS[5] + pSN[4] + pSN[5] + round)>>2;
			pD[5] = (pS[5] + pS[6] + pSN[5] + pSN[6] + round)>>2;
			pD[6] = (pS[6] + pS[7] + pSN[6] + pSN[7] + round)>>2;
			pD[7] = (pS[7] + pS[8] + pSN[7] + pSN[8] + round)>>2;
			pD[8] = (pS[8] + pS[9] + pSN[8] + pSN[9] + round)>>2;
			pD[9] = (pS[9] + pS[10] + pSN[9] + pSN[10] + round)>>2;
			pD[10] = (pS[10] + pS[11] + pSN[10] + pSN[11] + round)>>2;
			pD[11] = (pS[11] + pS[12] + pSN[11] + pSN[12] + round)>>2;
			pD[12] = (pS[12] + pS[13] + pSN[12] + pSN[13] + round)>>2;
			pD[13] = (pS[13] + pS[14] + pSN[13] + pSN[14] + round)>>2;
			pD[14] = (pS[14] + pS[15] + pSN[14] + pSN[15] + round)>>2;
			pD[15] = (pS[15] + pS[16] + pSN[15] + pSN[16] + round)>>2;
			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}	
	}

	return ippStsNoErr;
}


/*
this function does the way just same as ippiCopy8x8HP_8u_C1R
*/
IppStatus __STDCALL 
ippiCopy16x16HP_8u_C1R_c(							
						  const Ipp8u* pSrc,
						  int          srcStep,
						  Ipp8u*       pDst,
						  int          dstStep,
						  int          acc,
						  int          rounding)
{
	int i;
	register int round;
	Ipp8u *pS, *pD;

	round = 1 - rounding;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy16x16HP_8u_C1R		with acc(0,1,2,3): ",acc);
		dump("ippiCopy16x16HP_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	pS = (Ipp8u*)pSrc;
	pD = pDst;

	if (acc == 0)
	{
		if (3 & (int)pS)
		{
			// misaligned source
			for (i = 16; i > 0; i--)
			{
				pD[0] = pS[0];
				pD[1] = pS[1];
				pD[2] = pS[2];
				pD[3] = pS[3];
				pD[4] = pS[4];
				pD[5] = pS[5];
				pD[6] = pS[6];
				pD[7] = pS[7];
				pD[8] = pS[8];
				pD[9] = pS[9];
				pD[10] = pS[10];
				pD[11] = pS[11];
				pD[12] = pS[12];
				pD[13] = pS[13];
				pD[14] = pS[14];
				pD[15] = pS[15];

				pD += dstStep;
				pS += srcStep;
			}
		}
		else
		{
			// aligned source
			for (i = 16; i > 0; i--)
			{
				*(int *)(pD + 0) = *(int *)(pS + 0);
				*(int *)(pD + 4) = *(int *)(pS + 4);
				*(int *)(pD + 8) = *(int *)(pS + 8);
				*(int *)(pD + 12) = *(int *)(pS + 12);

				pD += dstStep;
				pS += srcStep;
			}
		}
	}
	else if (acc == 1)
	{

		for (i = 16; i > 0; i--)
		{
#if 0
			pD[0] = (pS[0] + pS[1] + round)>>1;
			pD[1] = (pS[1] + pS[2] + round)>>1;
			pD[2] = (pS[2] + pS[3] + round)>>1;
			pD[3] = (pS[3] + pS[4] + round)>>1;
			pD[4] = (pS[4] + pS[5] + round)>>1;
			pD[5] = (pS[5] + pS[6] + round)>>1;
			pD[6] = (pS[6] + pS[7] + round)>>1;
			pD[7] = (pS[7] + pS[8] + round)>>1;
			pD[8] = (pS[8] + pS[9] + round)>>1;
			pD[9] = (pS[9] + pS[10] + round)>>1;
			pD[10] = (pS[10] + pS[11] + round)>>1;
			pD[11] = (pS[11] + pS[12] + round)>>1;
			pD[12] = (pS[12] + pS[13] + round)>>1;
			pD[13] = (pS[13] + pS[14] + round)>>1;
			pD[14] = (pS[14] + pS[15] + round)>>1;
			pD[15] = (pS[15] + pS[16] + round)>>1;
#else
			int s0, s1;

			s0 = pS[1] + round;
			pD[0] = (pS[0] + s0) >> 1;
			s1 = pS[2];
			pD[1] = (s0 + s1) >> 1;
			s0 = pS[3] + round;
			pD[2] = (s0 + s1) >> 1;
			s1 = pS[4];
			pD[3] = (s0 + s1) >> 1;
			s0 = pS[5] + round;
			pD[4] = (s0 + s1) >> 1;
			s1 = pS[6];
			pD[5] = (s0 + s1) >> 1;
			s0 = pS[7] + round;
			pD[6] = (s0 + s1) >> 1;
			s1 = pS[8];
			pD[7] = (s0 + s1) >> 1;
			s0 = pS[9] + round;
			pD[8] = (s0 + s1) >> 1;
			s1 = pS[10];
			pD[9] = (s0 + s1) >> 1;
			s0 = pS[11] + round;
			pD[10] = (s0 + s1) >> 1;
			s1 = pS[12];
			pD[11] = (s0 + s1) >> 1;
			s0 = pS[13] + round;
			pD[12] = (s0 + s1) >> 1;
			s1 = pS[14];
			pD[13] = (s0 + s1) >> 1;
			s0 = pS[15] + round;
			pD[14] = (s0 + s1) >> 1;
			pD[15] = (s0 + pS[16]) >> 1;
#endif
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 2)
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;

		for (i = 16; i > 0; i--)
		{
			pD[0] = (pS[0] + pSN[0] + round)>>1;
			pD[1] = (pS[1] + pSN[1] + round)>>1;
			pD[2] = (pS[2] + pSN[2] + round)>>1;
			pD[3] = (pS[3] + pSN[3] + round)>>1;
			pD[4] = (pS[4] + pSN[4] + round)>>1;
			pD[5] = (pS[5] + pSN[5] + round)>>1;
			pD[6] = (pS[6] + pSN[6] + round)>>1;
			pD[7] = (pS[7] + pSN[7] + round)>>1;
			pD[8] = (pS[8] + pSN[8] + round)>>1;
			pD[9] = (pS[9] + pSN[9] + round)>>1;
			pD[10] = (pS[10] + pSN[10] + round)>>1;
			pD[11] = (pS[11] + pSN[11] + round)>>1;
			pD[12] = (pS[12] + pSN[12] + round)>>1;
			pD[13] = (pS[13] + pSN[13] + round)>>1;
			pD[14] = (pS[14] + pSN[14] + round)>>1;
			pD[15] = (pS[15] + pSN[15] + round)>>1;			
			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 3)
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;
		round = 2 - rounding;
		for (i = 16; i > 0; i--)
		{
#if 0
			pD[0] = (pS[0] + pS[1] + pSN[0] + pSN[1] + round)>>2;
			pD[1] = (pS[1] + pS[2] + pSN[1] + pSN[2] + round)>>2;
			pD[2] = (pS[2] + pS[3] + pSN[2] + pSN[3] + round)>>2;
			pD[3] = (pS[3] + pS[4] + pSN[3] + pSN[4] + round)>>2;
			pD[4] = (pS[4] + pS[5] + pSN[4] + pSN[5] + round)>>2;
			pD[5] = (pS[5] + pS[6] + pSN[5] + pSN[6] + round)>>2;
			pD[6] = (pS[6] + pS[7] + pSN[6] + pSN[7] + round)>>2;
			pD[7] = (pS[7] + pS[8] + pSN[7] + pSN[8] + round)>>2;
			pD[8] = (pS[8] + pS[9] + pSN[8] + pSN[9] + round)>>2;
			pD[9] = (pS[9] + pS[10] + pSN[9] + pSN[10] + round)>>2;
			pD[10] = (pS[10] + pS[11] + pSN[10] + pSN[11] + round)>>2;
			pD[11] = (pS[11] + pS[12] + pSN[11] + pSN[12] + round)>>2;
			pD[12] = (pS[12] + pS[13] + pSN[12] + pSN[13] + round)>>2;
			pD[13] = (pS[13] + pS[14] + pSN[13] + pSN[14] + round)>>2;
			pD[14] = (pS[14] + pS[15] + pSN[14] + pSN[15] + round)>>2;
			pD[15] = (pS[15] + pS[16] + pSN[15] + pSN[16] + round)>>2;
#else
			int s0, s1;

			s0 = pS[1] + pSN[1] + round;
			pD[0] = (pS[0] + pSN[0] + s0) >> 2;
			s1 = pS[2] + pSN[2];
			pD[1] = (s0 + s1) >> 2;
			s0 = pS[3] + pSN[3] + round;
			pD[2] = (s0 + s1) >> 2;
			s1 = pS[4] + pSN[4];
			pD[3] = (s0 + s1) >> 2;
			s0 = pS[5] + pSN[5] + round;
			pD[4] = (s0 + s1) >> 2;
			s1 = pS[6] + pSN[6];
			pD[5] = (s0 + s1) >> 2;
			s0 = pS[7] + pSN[7] + round;
			pD[6] = (s0 + s1) >> 2;
			s1 = pS[8] + pSN[8];
			pD[7] = (s0 + s1) >> 2;
			s0 = pS[9] + pSN[9] + round;
			pD[8] = (s0 + s1) >> 2;
			s1 = pS[10] + pSN[10];
			pD[9] = (s0 + s1) >> 2;
			s0 = pS[11] + pSN[11] + round;
			pD[10] = (s0 + s1) >> 2;
			s1 = pS[12] + pSN[12];
			pD[11] = (s0 + s1) >> 2;
			s0 = pS[13] + pSN[13] + round;
			pD[12] = (s0 + s1) >> 2;
			s1 = pS[14] + pSN[14];
			pD[13] = (s0 + s1) >> 2;
			s0 = pS[15] + pSN[15] + round;
			pD[14] = (s0 + s1) >> 2;
			pD[15] = (s0 + pS[16] + pSN[16]) >> 2;
#endif

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}
	}
	return ippStsNoErr;
}


/*
declared in ippvc.h
Copy8x8QP_MPEG4,		
Copy16x8QP_MPEG4,
Copy16x16QP_MPEG4
Copy fixed size blocks with quarter-pixel accuracy.
*/
IppStatus __STDCALL 
ippiCopy8x8QP_MPEG4_8u_C1R_c(
							  const Ipp8u* pSrc,
							  int          srcStep,
							  Ipp8u*       pDst,
							  int          dstStep,
							  int          acc,
							  int          rounding)
{
	int xFrac, yFrac;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	xFrac = 3&acc;
	yFrac = acc>>2;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy8x8QP_MPEG4_8u_C1R		with acc(0--15): ",acc);
		dump("ippiCopy8x8QP_MPEG4_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	/* direct copy */
	if (0 == acc)
	{
		int j;
		Ipp8u *pS = (Ipp8u *)pSrc;
		Ipp8u *pD = pDst;
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (j = 8; j > 0; j--)
		{
#if defined(WIN32)&&!defined(_WIN32_WCE)
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*(p1++) = *(p2++);
			*p1 = *p2;
#else		
			/*
			pD[0] = pS[0];
			pD[1] = pS[1];
			pD[2] = pS[2];
			pD[3] = pS[3];
			pD[4] = pS[4];
			pD[5] = pS[5];
			pD[6] = pS[6];
			pD[7] = pS[7];*/
			memcpy(pD,pS,8);
#endif
			pD += dstStep;
			pS += srcStep;
		}
	}
	/* only horizontal interpolation */
	else if (0 == yFrac)
	{
		calc_QP_only_h(pSrc,srcStep,9,9,pDst,dstStep,8,8,xFrac,rounding);
	}
	/* only vertical interpolation */
	else if (0 == xFrac)
	{
		calc_QP_only_v(pSrc,srcStep,9,9,pDst,dstStep,8,8,yFrac,rounding);
	}
	else	/* bias interpolation */
	{
		calc_QP_bias(pSrc,srcStep,9,9,pDst,dstStep,8,8,xFrac,yFrac,rounding);
	}

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiCopy16x8QP_MPEG4_8u_C1R_c(
							   const Ipp8u* pSrc,
							   int          srcStep,
							   Ipp8u*       pDst,
							   int          dstStep,
							   int          acc,
							   int          rounding)
{
	int xFrac, yFrac;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	xFrac = 3&acc;
	yFrac = acc>>2;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy16x8QP_MPEG4_8u_C1R		with acc(0--15): ",acc);
		dump("ippiCopy16x8QP_MPEG4_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	/* direct copy */
	if (0 == acc)
	{
		int j;
		Ipp8u *pS = (Ipp8u *)pSrc;
		Ipp8u *pD = pDst;
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (j = 8; j > 0; j--)
		{
#if defined(WIN32)&&!defined(_WIN32_WCE)
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*p1 = *p2;
#else
			/*
			pD[0] = pS[0];
			pD[1] = pS[1];
			pD[2] = pS[2];
			pD[3] = pS[3];
			pD[4] = pS[4];
			pD[5] = pS[5];
			pD[6] = pS[6];
			pD[7] = pS[7];
			pD[8] = pS[8];
			pD[9] = pS[9];
			pD[10] = pS[10];
			pD[11] = pS[11];
			pD[12] = pS[12];
			pD[13] = pS[13];
			pD[14] = pS[14];
			pD[15] = pS[15];
			*/
			memcpy(pD,pS,16);
#endif
			pD += dstStep;
			pS += srcStep;
		}
	}
	/* only horizontal interpolation */
	else if (0 == yFrac)
	{
		calc_QP_only_h(pSrc,srcStep,17,9,pDst,dstStep,16,8,xFrac,rounding);
	}
	/* only vertical interpolation */
	else if (0 == xFrac)
	{
		calc_QP_only_v(pSrc,srcStep,17,9,pDst,dstStep,16,8,yFrac,rounding);
	}
	else	/* bias interpolation */
	{
		calc_QP_bias(pSrc,srcStep,17,9,pDst,dstStep,16,8,xFrac,yFrac,rounding);
	}

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiCopy16x16QP_MPEG4_8u_C1R_c(
								const Ipp8u* pSrc,
								int          srcStep,
								Ipp8u*       pDst,
								int          dstStep,
								int          acc,
								int          rounding)
{
	int xFrac, yFrac;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	xFrac = 3&acc;
	yFrac = acc>>2;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy16x16QP_MPEG4_8u_C1R		with acc(0--15): ",acc);
		dump("ippiCopy16x16QP_MPEG4_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	/* direct copy */
	if (0 == acc)	
	{
		int j;
		Ipp8u *pS = (Ipp8u *)pSrc;
		Ipp8u *pD = pDst;
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (j = 16; j > 0; j--)
		{
#if defined(WIN32)&&!defined(_WIN32_WCE)
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*p1 = *p2;
#else
			/*
			pD[0] = pS[0];
			pD[1] = pS[1];
			pD[2] = pS[2];
			pD[3] = pS[3];
			pD[4] = pS[4];
			pD[5] = pS[5];
			pD[6] = pS[6];
			pD[7] = pS[7];
			pD[8] = pS[8];
			pD[9] = pS[9];
			pD[10] = pS[10];
			pD[11] = pS[11];
			pD[12] = pS[12];
			pD[13] = pS[13];
			pD[14] = pS[14];
			pD[15] = pS[15];
			*/
			memcpy(pD,pS,16);
#endif
			pD += dstStep;
			pS += srcStep;
		}
	}
	/* only horizontal interpolation */
	else if (0 == yFrac)
	{
		calc_QP_only_h(pSrc,srcStep,17,17,pDst,dstStep,16,16,xFrac,rounding);
	}
	/* only vertical interpolation */
	else if (0 == xFrac)
	{
		calc_QP_only_v(pSrc,srcStep,17,17,pDst,dstStep,16,16,yFrac,rounding);
	}
	else	/* bias interpolation */
	{
		calc_QP_bias(pSrc,srcStep,17,17,pDst,dstStep,16,16,xFrac,yFrac,rounding);
	}

	return ippStsNoErr;
}

