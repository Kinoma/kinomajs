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
//#include "Fsk.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"

IppStatus (__STDCALL *ippiAdd8x8HP_16s8u_C1RS_universal)			(const Ipp16s* pSrc1,int src1Step,Ipp8u* pSrc2,int src2Step,Ipp8u* pDst,int dstStep, int acc, int rounding)=NULL;
IppStatus (__STDCALL *ippiAdd8x8_16s8u_C1IRS_universal)				(const Ipp16s* pSrc, int srcStep,Ipp8u* pSrcDst,int srcDstStep)=NULL;
IppStatus (__STDCALL *ippiAverage8x8_8u_C1IR_universal)				(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep)=NULL;
IppStatus (__STDCALL *ippiAverage16x16_8u_C1IR_universal)			(const Ipp8u*  pSrc,int srcStep,Ipp8u* pSrcDst,int srcDstStep)=NULL;
IppStatus (__STDCALL *ippiMeanAbsDev16x16_8u32s_C1R_universal)		(const Ipp8u*  pSrc,  int srcStep, Ipp32s* pDst)=NULL;
IppStatus (__STDCALL *ippiSubSAD8x8_8u16s_C1R_universal) 			(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, int src2Step, Ipp16s* pDst, int dstStep, Ipp32s* pSAD)=NULL;
IppStatus (__STDCALL *ippiSubSAD8x8_8u16s_C1R_16x16_universal) 		(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD)=NULL;
IppStatus (__STDCALL *ippiSubSAD8x8_8u16s_C1R_8x16_universal) 		(const Ipp8u* pSrc1, int src1Step, const Ipp8u* pSrc2, Ipp16s* pDst, Ipp32s* pSAD)=NULL;
IppStatus (__STDCALL *ippiFrameFieldSAD16x16_8u32s_C1R_universal) 	(const Ipp8u* pSrc, int  srcStep,	Ipp32s*  pFrameSAD,  Ipp32s*  pFieldSAD)=NULL;
IppStatus (__STDCALL *ippiFrameFieldSAD16x16_16s32s_C1R_universal) 	(const Ipp16s* pSrc,  int  srcStep, Ipp32s* pFrameSAD, Ipp32s* pFieldSAD)=NULL;
IppStatus (__STDCALL *ippiSub8x8_8u16s_C1R_universal) 				(const Ipp8u*  pSrc1,  int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst,  int  dstStep)=NULL;
IppStatus (__STDCALL *ippiSub16x16_8u16s_C1R_universal) 			(const Ipp8u*  pSrc1, int src1Step, const Ipp8u*  pSrc2, int   src2Step, Ipp16s*  pDst, int  dstStep)=NULL;
IppStatus (__STDCALL *ippiCountZeros8x8_16s_C1_universal) 			(Ipp16s* pSrc, Ipp32u* pCount)=NULL;
IppStatus (__STDCALL *ippiCopy8x8_8u_C1R_universal)					(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep)=NULL;
IppStatus (__STDCALL *ippiCopy16x16_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep)=NULL;
IppStatus (__STDCALL *ippiSqrDiff16x16_8u32s_universal) 			(const Ipp8u*  pSrc,Ipp32s  srcStep,const Ipp8u*  pRef, Ipp32s  refStep,Ipp32s  mcType,Ipp32s* pSqrDiff)=NULL;
IppStatus (__STDCALL *ippiSSD8x8_8u32s_C1R_universal) 				(const Ipp8u  *pSrcCur,  int srcCurStep, const Ipp8u  *pSrcRef, int srcRefStep, Ipp32s *pDst, Ipp32s mcType)=NULL;


IppStatus __STDCALL 
ippiAdd8x8HP_16s8u_C1RS_c(								
    const Ipp16s* pSrc1,
    int           src1Step,
    Ipp8u*        pSrc2,
    int           src2Step,
    Ipp8u*        pDst,
    int           dstStep,
    int           acc,
    int           rounding)
{
	#define doClip(val, limit) if (((unsigned int) val) > limit) val = limit & ~(((signed int) val) >> 31)
	Ipp8u *pD;
	Ipp16s *pS1;
	int i,halfstep;
	unsigned int k255 = 255;
	ippiCopy8x8HP_8u_C1R_c(pSrc2,src2Step,pDst,dstStep,acc,rounding);
	
	halfstep = src1Step>>1;

	pD = pDst;
	pS1 = (Ipp16s*)pSrc1;
	for (i = 8; i > 0; i--)
	{
		int t;

		t = pD[0] + pS1[0]; doClip(t, k255); pD[0] = t;
		t = pD[1] + pS1[1]; doClip(t, k255); pD[1] = t;
		t = pD[2] + pS1[2]; doClip(t, k255); pD[2] = t;
		t = pD[3] + pS1[3]; doClip(t, k255); pD[3] = t;
		t = pD[4] + pS1[4]; doClip(t, k255); pD[4] = t;
		t = pD[5] + pS1[5]; doClip(t, k255); pD[5] = t;
		t = pD[6] + pS1[6]; doClip(t, k255); pD[6] = t;
		t = pD[7] + pS1[7]; doClip(t, k255); pD[7] = t;

		pS1 += halfstep;
		pD += dstStep;
	}

#ifdef PRINT_REF_INFO
	{
		dump("ippiAdd8x8HP_16s8u_C1RS		 with acc(0,1,2,3): ",acc);
		dump("ippiAdd8x8HP_16s8u_C1RS		 with rounding(0,1): ",rounding);
	}
#endif

#if 0
	int i,halfstep;
	register int round = 1 - rounding;
	Ipp16s *pS1;
	register Ipp16s tmp;
	Ipp8u *pS2, *pD;

	if (!pSrc1 || !pSrc2 || !pDst)
		return ippStsNullPtrErr;

	pS2 = pSrc2;
	pD = pDst;
	pS1 = (Ipp16s*)pSrc1;
	halfstep = src1Step>>1;
	
	if (acc == 0)
	{
		for (i = 8; i > 0; i--)
		{
			pD[0] = CLIP(pS2[0] + pS1[0]);
			pD[1] = CLIP(pS2[1] + pS1[1]);
			pD[2] = CLIP(pS2[2] + pS1[2]);
			pD[3] = CLIP(pS2[3] + pS1[3]);
			pD[4] = CLIP(pS2[4] + pS1[4]);
			pD[5] = CLIP(pS2[5] + pS1[5]);
			pD[6] = CLIP(pS2[6] + pS1[6]);
			pD[7] = CLIP(pS2[7] + pS1[7]);

			pS1 += halfstep;
			pS2 += src2Step;
			pD += dstStep;
		}
	}
	else if (acc == 1)
	{
		for (i = 8; i > 0; i--)
		{
			tmp = pS1[0] + ((pS2[0] + pS2[1] + round)>>1);
			pD[0] = CLIP(tmp);
			tmp = pS1[1] + ((pS2[1] + pS2[2] + round)>>1);
			pD[1] = CLIP(tmp);
			tmp = pS1[2] + ((pS2[2] + pS2[3] + round)>>1);
			pD[2] = CLIP(tmp);
			tmp = pS1[3] + ((pS2[3] + pS2[4] + round)>>1);
			pD[3] = CLIP(tmp);
			tmp = pS1[4] + ((pS2[4] + pS2[5] + round)>>1);
			pD[4] = CLIP(tmp);
			tmp = pS1[5] + ((pS2[5] + pS2[6] + round)>>1);
			pD[5] = CLIP(tmp);
			tmp = pS1[6] + ((pS2[6] + pS2[7] + round)>>1);
			pD[6] = CLIP(tmp);
			tmp = pS1[7] + ((pS2[7] + pS2[8] + round)>>1);
			pD[7] = CLIP(tmp);

			pS1 += halfstep;
			pS2 += src2Step;
			pD += dstStep;
		}
	}
	else if (acc == 2)
	{
		Ipp8u *pS2N = pSrc2+src2Step;
		for (i = 8; i > 0; i--)
		{
			tmp = pS1[0] + ((pS2[0] + pS2N[0] + round)>>1);
			pD[0] = CLIP(tmp);
			tmp = pS1[1] + ((pS2[1] + pS2N[1] + round)>>1);
			pD[1] = CLIP(tmp);
			tmp = pS1[2] + ((pS2[2] + pS2N[2] + round)>>1);
			pD[2] = CLIP(tmp);
			tmp = pS1[3] + ((pS2[3] + pS2N[3] + round)>>1);
			pD[3] = CLIP(tmp);
			tmp = pS1[4] + ((pS2[4] + pS2N[4] + round)>>1);
			pD[4] = CLIP(tmp);
			tmp = pS1[5] + ((pS2[5] + pS2N[5] + round)>>1);
			pD[5] = CLIP(tmp);
			tmp = pS1[6] + ((pS2[6] + pS2N[6] + round)>>1);
			pD[6] = CLIP(tmp);
			tmp = pS1[7] + ((pS2[7] + pS2N[7] + round)>>1);
			pD[7] = CLIP(tmp);

			pS1 += halfstep;
			pS2 += src2Step;
			pS2N += src2Step;
			pD += dstStep;
		}

	}
	else if (acc == 3)
	{
		Ipp8u *pS2N = pSrc2+src2Step;
		round = 2 - rounding;
		for (i = 8; i >0; i--)
		{
			tmp = pS1[0] + ((pS2[0] + pS2[1] + pS2N[0] + pS2N[1] + round)>>2);
			pD[0] = CLIP(tmp);
			tmp = pS1[1] + ((pS2[1] + pS2[2] + pS2N[1] + pS2N[2] + round)>>2);
			pD[1] = CLIP(tmp);
			tmp = pS1[2] + ((pS2[2] + pS2[3] + pS2N[2] + pS2N[3] + round)>>2);
			pD[2] = CLIP(tmp);
			tmp = pS1[3] + ((pS2[3] + pS2[4] + pS2N[3] + pS2N[4] + round)>>2);
			pD[3] = CLIP(tmp);
			tmp = pS1[4] + ((pS2[4] + pS2[5] + pS2N[4] + pS2N[5] + round)>>2);
			pD[4] = CLIP(tmp);
			tmp = pS1[5] + ((pS2[5] + pS2[6] + pS2N[5] + pS2N[6] + round)>>2);
			pD[5] = CLIP(tmp);
			tmp = pS1[6] + ((pS2[6] + pS2[7] + pS2N[6] + pS2N[7] + round)>>2);
			pD[6] = CLIP(tmp);
			tmp = pS1[7] + ((pS2[7] + pS2[8] + pS2N[7] + pS2N[8] + round)>>2);
			pD[7] = CLIP(tmp);

			pS1 += halfstep;
			pS2 += src2Step;
			pS2N += src2Step;
			pD += dstStep;
		}
	}
#endif
	return ippStsNoErr;
}

//#endif

IppStatus __STDCALL 
ippiAdd8x8_16s8u_C1IRS_c(
						  const Ipp16s* pSrc,
						  int           srcStep,
						  Ipp8u*        pSrcDst,
						  int           srcDstStep)
{
	int i, halfstep;
	Ipp16s *pS;
	Ipp8u  *pD;
	/*register Ipp16s tmp;*/

	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	dump("ippiAdd8x8_16s8u_C1IRS", -1);
#endif

	pS = (Ipp16s*)pSrc;
	pD = pSrcDst;
	halfstep = srcStep>>1;

	for(i = 8; i > 0; i--)
	{
		pD[0] = CLIP(pS[0] + pD[0]);
		pD[1] = CLIP(pS[1] + pD[1]);
		pD[2] = CLIP(pS[2] + pD[2]);
		pD[3] = CLIP(pS[3] + pD[3]);
		pD[4] = CLIP(pS[4] + pD[4]);
		pD[5] = CLIP(pS[5] + pD[5]);
		pD[6] = CLIP(pS[6] + pD[6]);
		pD[7] = CLIP(pS[7] + pD[7]);

		/*
		tmp = (pS[0] + pD[0]);
		pD[0] = CLIP(tmp);
		tmp = (pS[1] + pD[1]);
		pD[1] = CLIP(tmp);
		tmp = (pS[2] + pD[2]);
		pD[2] = CLIP(tmp);
		tmp = (pS[3] + pD[3]);
		pD[3] = CLIP(tmp);
		tmp = (pS[4] + pD[4]);
		pD[4] = CLIP(tmp);
		tmp = (pS[5] + pD[5]);
		pD[5] = CLIP(tmp);
		tmp = (pS[6] + pD[6]);
		pD[6] = CLIP(tmp);
		tmp = (pS[7] + pD[7]);
		pD[7] = CLIP(tmp);
		*/

		pS += halfstep;
		pD += srcDstStep;
	}

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiAverage8x8_8u_C1IR_c(
						  const Ipp8u*  pSrc,
						  int           srcStep,
						  Ipp8u*        pSrcDst,
						  int           srcDstStep)
{
	int i;
	Ipp8u *pS, *pD;

	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	dump("ippiAverage8x8_8u_C1IR" ,-1);
#endif

	pS = (Ipp8u*)pSrc;
	pD = pSrcDst;

	for(i = 8; i > 0; i--)
	{
		pD[0] = (pS[0] + pD[0] + 1) >> 1;
		pD[1] = (pS[1] + pD[1] + 1) >> 1;
		pD[2] = (pS[2] + pD[2] + 1) >> 1;
		pD[3] = (pS[3] + pD[3] + 1) >> 1;
		pD[4] = (pS[4] + pD[4] + 1) >> 1;
		pD[5] = (pS[5] + pD[5] + 1) >> 1;
		pD[6] = (pS[6] + pD[6] + 1) >> 1;
		pD[7] = (pS[7] + pD[7] + 1) >> 1;

		pS += srcStep;
		pD += srcDstStep;
	}

	return ippStsNoErr;
}

IppStatus __STDCALL 
ippiAverage16x16_8u_C1IR_c(
							const Ipp8u*  pSrc,
							int           srcStep,
							Ipp8u*        pSrcDst,
							int           srcDstStep)
{
	int i;
	Ipp8u *pS, *pD;

	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	dump("ippiAverage16x16_8u_C1IR", -1);
#endif

	pS = (Ipp8u*)pSrc;
	pD = pSrcDst;

	for(i = 16; i > 0; i--)
	{
		pD[0] = (pS[0] + pD[0] + 1) >> 1;
		pD[1] = (pS[1] + pD[1] + 1) >> 1;
		pD[2] = (pS[2] + pD[2] + 1) >> 1;
		pD[3] = (pS[3] + pD[3] + 1) >> 1;
		pD[4] = (pS[4] + pD[4] + 1) >> 1;
		pD[5] = (pS[5] + pD[5] + 1) >> 1;
		pD[6] = (pS[6] + pD[6] + 1) >> 1;
		pD[7] = (pS[7] + pD[7] + 1) >> 1;

		pD[8] = (pS[8] + pD[8] + 1) >> 1;
		pD[9] = (pS[9] + pD[9] + 1) >> 1;
		pD[10] = (pS[10] + pD[10] + 1) >> 1;
		pD[11] = (pS[11] + pD[11] + 1) >> 1;
		pD[12] = (pS[12] + pD[12] + 1) >> 1;
		pD[13] = (pS[13] + pD[13] + 1) >> 1;
		pD[14] = (pS[14] + pD[14] + 1) >> 1;
		pD[15] = (pS[15] + pD[15] + 1) >> 1;

		pS += srcStep;
		pD += srcDstStep;
	}

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiCopy8x8_8u_C1R_c(					
					  const Ipp8u* pSrc,
					  int          srcStep,
					  Ipp8u*       pDst,
					  int          dstStep)
{
	int i;
	Ipp8u *pS, *pD;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	dump("ippiCopy8x8_8u_C1R", -1);
#endif

	pS = (Ipp8u*)pSrc;
	pD = pDst;
	for (i = 8; i > 0; i--)
	{
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
		pS += srcStep;
		pD += dstStep;
	}

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiCopy16x16_8u_C1R_c(							
						const Ipp8u* pSrc,
						int          srcStep,
						Ipp8u*       pDst,
						int          dstStep)
{
	int i;
	Ipp8u *pS, *pD;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	dump("ippiCopy16x16_8u_C1R", -1);
#endif

	pS = (Ipp8u*)pSrc;
	pD = pDst;
	for (i = 16; i > 0; i--)
	{
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
		pS += srcStep;
		pD += dstStep;
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_16x16_c
(
	const Ipp8u*	pS1,  
	int				src1Step,  
	const Ipp8u*	pS2,
	Ipp16s*			pD,  
	Ipp32s*			pSAD
)
{
	int j, sum = 0;
	int src_stride = src1Step - 8;

	for (j = 8; j > 0; j--)
	{
		int diff1;
		int diff2;
		
		//
		diff1 = *pS1++ - *pS2++;
		diff2 = *pS1++ - *pS2++;

		sum += abs(diff1); sum += abs(diff2);
		diff1 &= 0xffff; diff2 = (diff2<<16)|diff1;
		*((long *)pD) = diff2; pD += 2;

		//
		diff1 = *pS1++ - *pS2++;
		diff2 = *pS1++ - *pS2++;

		sum += abs(diff1); sum += abs(diff2);
		diff1 &= 0xffff; diff2 = (diff2<<16)|diff1;
		*((long *)pD) = diff2; pD += 2;

		//
		diff1 = *pS1++ - *pS2++;
		diff2 = *pS1++ - *pS2++;

		sum += abs(diff1); sum += abs(diff2);
		diff1 &= 0xffff; diff2 = (diff2<<16)|diff1;
		*((long *)pD) = diff2; pD += 2;

		//
		diff1 = *pS1++ - *pS2++;
		diff2 = *pS1++ - *pS2++;

		sum += abs(diff1); sum += abs(diff2);
		diff1 &= 0xffff; diff2 = (diff2<<16)|diff1;
		*((long *)pD) = diff2; pD += 2;

		pS1 += src_stride;
		pS2 += 8;
	}
	*pSAD = sum;

	return ippStsNoErr;
}

IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_8x16_c
(
	const Ipp8u*	pS1,  
	int				src1Step,  
	const Ipp8u*	pS2,
	Ipp16s*			pD,  
	Ipp32s*			pSAD
)
{
	int i,j, sum = 0;
	int src_stride = src1Step - 8;

	for (j = 8; j > 0; j--)
	{
		for (i = 0; i < 4; i++)
		{
			int diff1 = *pS1++ - *pS2++;
			int diff2 = *pS1++ - *pS2++;

			sum += abs(diff1);
			sum += abs(diff2);

			diff1 &= 0xffff;
			diff2 = (diff2<<16)|diff1;
			*((long *)pD) = diff2;
			pD += 2;
		}

		pS1 += src_stride;
	}
	*pSAD = sum;

	return ippStsNoErr;
}

#ifdef __INTEL_IPP__
IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_16x16
(
	const Ipp8u*	pSrc1,  
	int				src1Step,  
	const Ipp8u*	pSrc2,
	Ipp16s*			pDst,  
	Ipp32s*			pSAD
)
{
	return ippiSubSAD8x8_8u16s_C1R
			(
				pSrc1,  
				src1Step,  
				pSrc2,
				16,  
				pDst,  
				16,  
				pSAD
			);
}


IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_8x16
(
	const Ipp8u*	pSrc1,  
	int				src1Step,  
	const Ipp8u*	pSrc2,
	Ipp16s*			pDst,  
	Ipp32s*			pSAD
)
{
	return ippiSubSAD8x8_8u16s_C1R
			(
				pSrc1,  
				src1Step,  
				pSrc2,
				8,  
				pDst,  
				16,  
				pSAD
			);
}
#endif

IppStatus __STDCALL ippiSubSAD8x8_8u16s_C1R_c
(
	const Ipp8u*	pSrc1,  
	int				src1Step,  
	const Ipp8u*	pSrc2,
	int				src2Step,  
	Ipp16s*			pDst,  
	int				dstStep,  
	Ipp32s*			pSAD
)
{
#if 1
	int i,j, sum = 0;
	const Ipp8u *pS1, *pS2;
	Ipp16s *pD;
	if (!pSrc1 || ! pSrc2 || !pDst)
		return ippStsNullPtrErr;

	pS1 = pSrc1;
	pS2 = pSrc2;
	pD = pDst;
	dstStep >>= 1;

	for (j = 8; j > 0; j--)
	{
		for (i = 0; i < 8; i++)
		{
			pD[i] = (Ipp16s)pS1[i] - pS2[i];
			sum += abs(pD[i]);
		}
		pD += dstStep;
		pS1 += src1Step;
		pS2 += src2Step;
	}
	*pSAD = sum;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiSubSAD8x8_8u16s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiSubSAD8x8_8u16s_C1R(pSrc1, src1Step, pSrc2, src2Step, pDst, dstStep,pSAD);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiSub8x8_8u16s_C1R_c(
	const Ipp8u*  pSrc1,  int src1Step, const Ipp8u*  pSrc2,
	int   src2Step, Ipp16s*  pDst,  int  dstStep)
{
#if 1
	int i,j;
	const Ipp8u *pS1, *pS2;
	Ipp16s *pD;
	if (!pSrc1 || !pSrc2 || !pDst)
		return ippStsNullPtrErr;

	pS1 = pSrc1;
	pS2 = pSrc2;
	pD = pDst;
	dstStep = dstStep >> 1;
	for (j = 8; j > 0; j--)
	{
		for (i = 0; i < 8; i++)
		{
			pD[i] = (Ipp16s)pS1[i] - (Ipp16s)pS2[i];
		}
		pS1 += src1Step;
		pS2 += src2Step;
		pD  += dstStep;
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippiSub8x8_8u16s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiSub8x8_8u16s_C1R(pSrc1, src1Step, pSrc2, src2Step, pDst, dstStep);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiSub16x16_8u16s_C1R_c(
	const Ipp8u*  pSrc1, int src1Step, const Ipp8u*  pSrc2,
	int   src2Step, Ipp16s*  pDst, int  dstStep)
{
#if 1
	int i,j;
	const Ipp8u *pS1, *pS2;
	Ipp16s *pD;
	if (!pSrc1 || !pSrc2 || !pDst)
		return ippStsNullPtrErr;

	pS1 = pSrc1;
	pS2 = pSrc2;
	pD = pDst;
	dstStep = dstStep >> 1;
	for (j = 16; j > 0; j--)
	{
		for (i = 0; i < 16; i++)
		{
			pD[i] = (Ipp16s)pS1[i] - (Ipp16s)pS2[i];
		}
		pS1 += src1Step;
		pS2 += src2Step;
		pD  += dstStep;
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippiSub16x16_8u16s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiSub16x16_8u16s_C1R(pSrc1, src1Step,pSrc2,src2Step, pDst, dstStep);
		return sts;
	}
#endif

	return ippStsNoErr;
}



IppStatus __STDCALL ippiFrameFieldSAD16x16_16s32s_C1R_c(
	const Ipp16s* pSrc,  int  srcStep,
	Ipp32s*       pFrameSAD, Ipp32s*       pFieldSAD)
{
#if 1
	int i,j,step,dbStep, frameSum = 0, fieldSum = 0;
	const Ipp16s *pfm1, *pfm2;
	const Ipp16s *pfd1, *pfd2;
	if (!pSrc)
		return ippStsNullPtrErr;

	step = srcStep >> 1;
	dbStep = srcStep;
	pfm1 = pfd1 = pSrc;
	pfm2 = pSrc + step;
	pfd2 = pfd1 + dbStep;
	for (j = 7; j > 0; j--)
	{
		for (i = 0; i < 16; i++)
		{
			frameSum += abs((Ipp32s)pfm1[i] - pfm2[i]);
			fieldSum += abs((Ipp32s)pfd1[i] - pfd2[i]);
		}
		pfm1 += step;
		pfm2 += step;
		pfd1 += dbStep;
		pfd2 += dbStep;
	}

	pfm1 += step;
	pfm2 += step;
	pfd1 = pSrc + step;
	pfd2 = pfd1 + dbStep;

	for (j = 7; j > 0; j--)
	{
		for (i = 0; i < 16; i++)
		{
			frameSum += abs((Ipp32s)pfm1[i] - pfm2[i]);
			fieldSum += abs((Ipp32s)pfd1[i] - pfd2[i]);
		}
		pfm1 += step;
		pfm2 += step;
		pfd1 += dbStep;
		pfd2 += dbStep;
	}
	*pFrameSAD = frameSum;
	*pFieldSAD = fieldSum;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiFrameFieldSAD16x16_16s32s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiFrameFieldSAD16x16_16s32s_C1R(pSrc, srcStep, pFrameSAD,pFieldSAD);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiFrameFieldSAD16x16_8u32s_C1R_c(
	const Ipp8u* pSrc,  int  srcStep,
	Ipp32s*  pFrameSAD,  Ipp32s*  pFieldSAD)
{
#if 1
	int i,j,dbStep, frameSum = 0, fieldSum = 0;
	const Ipp8u *pfm1, *pfm2,  *pfd1, *pfd2;
	if (!pSrc)
		return ippStsNullPtrErr;

	pfm1 = pfd1 = pSrc;
	pfm2 = pSrc + srcStep;
	dbStep = srcStep << 1;
	pfd2 = pfd1 + dbStep;
	for (j = 7; j > 0; j--)
	{
		for (i = 0; i < 16; i++)
		{
			frameSum += abs((Ipp32s)pfm1[i] - pfm2[i]);
			fieldSum += abs((Ipp32s)pfd1[i] - pfd2[i]);
		}
		pfm1 += srcStep;
		pfm2 += srcStep;
		pfd1 += dbStep;
		pfd2 += dbStep;
	}

	pfm1 += srcStep;
	pfm2 += srcStep;
	pfd1 = pSrc + srcStep;
	pfd2 = pfd1 + dbStep;

	for (j = 7; j > 0; j--)
	{
		for (i = 0; i < 16; i++)
		{
			frameSum += abs((Ipp32s)pfm1[i] - pfm2[i]);
			fieldSum += abs((Ipp32s)pfd1[i] - pfd2[i]);
		}
		pfm1 += srcStep;
		pfm2 += srcStep;
		pfd1 += dbStep;
		pfd2 += dbStep;
	}
	*pFrameSAD = frameSum;
	*pFieldSAD = fieldSum;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiFrameFieldSAD16x16_8u32s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiFrameFieldSAD16x16_8u32s_C1R(pSrc, srcStep, pFrameSAD,pFieldSAD);
		return sts;
	}
#endif

	return ippStsNoErr;
}








/**************************** the following are not sure which belong to *************************/
//IppStatus __STDCALL 
//ippiCopy_8u_C1R_c(const Ipp8u *pSrc, int srcStep, Ipp8u *pDst, int dstStep, IppiSize roiSize)
//{
//#if 1
//	int i,j;
//	Ipp8u *pS, *pD;
//	if (!pSrc || !pDst)
//		return ippStsNullPtrErr;
//	if (roiSize.height < 1 || roiSize.width < 1)
//		return ippStsSizeErr;
//	
//	pS = pSrc;
//	pD = pDst;
//	for (j = roiSize.height; j > 0; j--)
//	{
//		for (i = 0; i < roiSize.width; i++)
//		{
//			pD[i] = pS[i];
//		}
//		pS += srcStep;
//		pD += dstStep;
//	}
//#endif
//
//#ifdef PRINT_REF_INFO
//		dump("ippiCopy_8u_C1R",-1);
//#endif
//
//#if 0
//	{
//		IppStatus sts;
//		sts = ippiCopy_8u_C1R(pSrc, srcStep, pDst, dstStep, roiSize);
//		return sts;
//	}
//#endif
//
//	return ippStsNoErr;
//}


IppStatus __STDCALL ippiCountZeros8x8_16s_C1_c(Ipp16s* pSrc, Ipp32u* pCount)
{
#if 1
	int i;
	if (!pSrc)
		return ippStsNullPtrErr;
	*pCount = 0;
	for (i = 0; i < 64; i++)
	{
		if (!pSrc[i])
			(*pCount)++;	//*pCount += !(pSrc[i] && 1);
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippiCountZeros8x8_16s_C1",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiCountZeros8x8_16s_C1(pSrc, pCount);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiMeanAbsDev16x16_8u32s_C1R_c(
	const Ipp8u*  pSrc,  int srcStep, Ipp32s* pDst)
{
#if 1
	int i,j, mean,sum;
	const Ipp8u *pS;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	pS = pSrc;
	sum = 0;
	for (j = 16; j > 0; j--)
	{
		for (i = 0; i < 16; i++)
		{
			sum += pS[i];
		}
		pS += srcStep;
	}
	mean = (sum+128) >> 8;  /* / 256;  */

	pS = pSrc;
	sum = 0;
	for (j = 16; j > 0; j--)
	{
		for (i = 0; i < 16; i++)
		{
			sum += abs(pS[i] - mean);
		}
		pS += srcStep;
	}
	*pDst = sum;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiMeanAbsDev16x16_8u32s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiMeanAbsDev16x16_8u32s_C1R(pSrc, srcStep, pDst);
		return sts;
	}
#endif

	return ippStsNoErr;
}

 static int __inline sadx(int width, int height, Ipp8u *pSC, int scStep, Ipp8u *pSR, int srStep)
 {
 	int i,j;
 	register int sum = 0;
 	for (j = height; j > 0; j--)
 	{
 		for (i = 0; i < width; i++)
 		{
 			sum += abs((int)pSC[i] - pSR[i]);
 		}
 		pSC += scStep;
 		pSR += srStep;
 	}
 	return sum;
 }
 static void __inline mc_apx_fh(int width, int height,  Ipp8u *pSR, int srStep, Ipp8u *pSRN)
 {
 	int i,j;
 	Ipp8u *pN, *pSR2;
 	pN = pSRN;
 	pSR2 = pSR + srStep;
 	for (j = height; j > 0; j--)
 	{
 		for (i = 0; i < width; i++)
 		{
 			pN[i] = (pSR[i] + pSR2[i] + 1) >> 1;
 		}
 		pN += width;
 		pSR += srStep;
 		pSR2 += srStep;
 	}
 }
 
 static void __inline mc_apx_hf(int width, int height,  Ipp8u *pSR, int srStep, Ipp8u *pSRN)
 {
 	int i,j;
 	Ipp8u *pN, *pSR2;
 	pN = pSRN;
 	pSR2 = pSR + 1;
 	for (j = height; j > 0; j--)
 	{
 		for (i = 0; i < width; i++)
 		{
 			pN[i] = (pSR[i] + pSR2[i] + 1) >> 1;
 		}
 		pN += width;
 		pSR += srStep;
 		pSR2 += srStep;
 	}
 }
 
 static void __inline mc_apx_hh(int width, int height,  Ipp8u *pSR, int srStep, Ipp8u *pSRN)
 {
 	int i,j;
 	Ipp8u *pN, *pSR2, *pSR3, *pSR4;
 	pN = pSRN;
 	pSR2 = pSR + 1;
 	pSR3 = pSR + srStep;
 	pSR4 = pSR3 + 1;
 	for (j = height; j > 0; j--)
 	{
 		for (i = 0; i < width; i++)
 		{
 			pN[i] = (pSR[i] + pSR2[i] + pSR3[i] + pSR4[i] + 2) >> 2;
 		}
 		pN += width;
 		pSR += srStep;
 		pSR2 += srStep;
 		pSR3 += srStep;
 		pSR4 += srStep;
 	}
 }
 
 /*this function is used in SqrDiff and SSD*/
 static int __inline sadxx(int width, int height, Ipp8u *pSC, int scStep, Ipp8u *pSR, int srStep)
 {
 	int i,j, tmp,sum = 0;
 	for (j = height; j > 0; j--)
 	{
 		for (i = 0; i < width; i++)
 		{
 			tmp = (Ipp32s)pSC[i] - pSR[i];
 			sum += tmp * tmp;
 		}
 		pSC += scStep;
 		pSR += srStep;
 	}
 	return sum;
 }
 
 IppStatus __STDCALL ippiSqrDiff16x16_8u32s_c(
 	const Ipp8u*  pSrc,Ipp32s  srcStep,const Ipp8u*  pRef,
 	Ipp32s  refStep,Ipp32s  mcType,Ipp32s* pSqrDiff)
 {
 #if 1
 	/* NOTICE:
 	the parameter "mcType" is always IPPVC_MC_APX_FF in Intel's sample code */
 	Ipp8u *pSC, *pSR;
 	if (!pSrc || ! pRef)
 		return ippStsNullPtrErr;
 	if (srcStep < 1 || refStep < 1)
 		return ippStsStepErr;
 
 	pSC = (Ipp8u *)pSrc;
 	pSR = (Ipp8u *)pRef;
 	switch(mcType)
 	{
 	case IPPVC_MC_APX_FF:	/* 0,	dst += abs(cur - ref) */
 		*pSqrDiff = sadxx(16,16,pSC,srcStep,pSR,refStep);
 		break;
 	case IPPVC_MC_APX_FH:	/* 4, */
 		{
 			Ipp8u SRNew[256];
 			//***bnie: printf("ippiSqrDiff16x16_8u32s:IPPVC_MC_APX_FH was not tested in intel's code, please test me firstly\n");
 			mc_apx_fh(16,16,pSR,refStep,SRNew);
 			*pSqrDiff = sadxx(16,16,pSC,srcStep,SRNew,16);
 			break;
 		}
 	case IPPVC_MC_APX_HF:	/* 8, */
 		{
 			Ipp8u SRNew[256];
 			//***bnie: printf("ippiSqrDiff16x16_8u32s:IPPVC_MC_APX_HF was not tested in intel's code, please test me firstly\n");
 			mc_apx_hf(16,16,pSR,refStep,SRNew);
 			*pSqrDiff = sadxx(16,16,pSC,srcStep,SRNew,16);
 			break;
 		}
 	case IPPVC_MC_APX_HH:	/* 12, */
 		{
 			Ipp8u SRNew[256];
 			//***bnie: printf("ippiSqrDiff16x16_8u32s:IPPVC_MC_APX_HH was not tested in intel's code, please test me firstly\n");
 			mc_apx_hh(16,16,pSR,refStep,SRNew);
 			*pSqrDiff = sadxx(16,16,pSC,srcStep,SRNew,16);
 			break;
 		}
 	default:break;
 	}
 
 #endif
 
 #ifdef PRINT_REF_INFO
 	dump("ippiSqrDiff16x16_8u32s",-1);
 #endif
 
 #if 0
 	{
 		IppStatus sts;
 		sts = ippiSqrDiff16x16_8u32s(pSrc,srcStep, pRef,refStep, mcType,pSqrDiff);
 		return sts;
 	}
 #endif
 
 	return ippStsNoErr;
 }
 
 
 IppStatus __STDCALL ippiSSD8x8_8u32s_C1R_c(
 	const Ipp8u  *pSrcCur,  int srcCurStep, const Ipp8u  *pSrcRef,
 	int   srcRefStep, Ipp32s *pDst, Ipp32s mcType)
 {
 #if 1
 	/* NOTICE:
 	the parameter "mcType" is always IPPVC_MC_APX_FF in Intel's sample code */
 	Ipp8u *pSC, *pSR;
 	if (!pSrcCur || ! pSrcRef)
 		return ippStsNullPtrErr;
 
 	pSC = (Ipp8u *)pSrcCur;
 	pSR = (Ipp8u *)pSrcRef;
 	switch(mcType)
 	{
 	case IPPVC_MC_APX_FF:	/* 0,	dst += abs(cur - ref) */
 		*pDst = sadxx(8,8,pSC,srcCurStep,pSR,srcRefStep);
 		break;
 	case IPPVC_MC_APX_FH:	/* 4, */
 		{
 			Ipp8u SRNew[64];
 			//***bnie: printf("ippiSSD8x8_8u32s_C1R:IPPVC_MC_APX_FH was not tested in intel's code, please test me firstly\n");
 			mc_apx_fh(8,8,pSR,srcRefStep,SRNew);
 			*pDst = sadxx(8,8,pSC,srcCurStep,SRNew,8);
 			break;
 		}
 	case IPPVC_MC_APX_HF:	/* 8, */
 		{
 			Ipp8u SRNew[64];
 			//***bnie: printf("ippiSSD8x8_8u32s_C1R:IPPVC_MC_APX_HF was not tested in intel's code, please test me firstly\n");
 			mc_apx_hf(8,8,pSR,srcRefStep,SRNew);
 			*pDst = sadxx(8,8,pSC,srcCurStep,SRNew,8);
 			break;
 		}
 	case IPPVC_MC_APX_HH:	/* 12, */
 		{
 			Ipp8u SRNew[64];
 			//***bnie: printf("ippiSSD8x8_8u32s_C1R:IPPVC_MC_APX_HH was not tested in intel's code, please test me firstly\n");
 			mc_apx_hh(8,8,pSR,srcRefStep,SRNew);
 			*pDst = sadxx(8,8,pSC,srcCurStep,SRNew,8);
 			break;
 		}
 	default:break;
 	}
 
 #endif
 
 #ifdef PRINT_REF_INFO
 	dump("ippiSSD8x8_8u32s_C1R",-1);
 #endif
 
 #if 0
 	{
 		IppStatus sts;
 		sts = ippiSSD8x8_8u32s_C1R(pSrcCur, srcCurStep,pSrcRef, srcRefStep, pDst,mcType);
 		return sts;
 	}
 #endif
 
 	return ippStsNoErr;
 }
