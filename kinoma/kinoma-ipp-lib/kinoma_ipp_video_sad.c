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


IppStatus (__STDCALL *ippiSAD16x16_8u32s_universal)					(const Ipp8u*  pSrc,  Ipp32s  srcStep, const Ipp8u*  pRef,	Ipp32s  refStep,  Ipp32s* pSAD, Ipp32s  mcType)=NULL;
IppStatus (__STDCALL *ippiSAD16x8_8u32s_C1R_universal) 				(const Ipp8u  *pSrcCur,  int  srcCurStep,  const Ipp8u  *pSrcRef,int srcRefStep, Ipp32s *pDst, Ipp32s mcType)=NULL;
IppStatus (__STDCALL *ippiSAD8x8_8u32s_C1R_universal) 				(const Ipp8u*  pSrcCur, int srcCurStep, const Ipp8u*  pSrcRef, int     srcRefStep, Ipp32s* pDst,Ipp32s  mcType)=NULL;


/* the following 4 inline functions are used in SAD functions */
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

IppStatus __STDCALL ippiSAD16x16_8u32s_c(
	const Ipp8u*  pSrc,  Ipp32s  srcStep, const Ipp8u*  pRef,
	Ipp32s  refStep,  Ipp32s* pSAD, Ipp32s  mcType)
{
#if 1

	Ipp8u *pSC, *pSR;
	if (!pSrc || ! pRef)
		return ippStsNullPtrErr;
	if (srcStep < 1 || refStep < 1)
		return ippStsStepErr;

	pSC = pSrc;
	pSR = pRef;
	switch(mcType)
	{
	case IPPVC_MC_APX_FF:	/* 0,	dst += abs(cur - ref) */
		*pSAD = sadx(16,16,pSC,srcStep,pSR,refStep);
		break;
	case IPPVC_MC_APX_FH:	/* 4, */
		{
			Ipp8u SRNew[256];
			mc_apx_fh(16,16,pSR,refStep,SRNew);
			*pSAD = sadx(16,16,pSC,srcStep,SRNew,16);
			break;
		}
	case IPPVC_MC_APX_HF:	/* 8, */
		{
			Ipp8u SRNew[256];
			mc_apx_hf(16,16,pSR,refStep,SRNew);
			*pSAD = sadx(16,16,pSC,srcStep,SRNew,16);
			break;
		}
	case IPPVC_MC_APX_HH:	/* 12, */
		{
			Ipp8u SRNew[256];
			mc_apx_hh(16,16,pSR,refStep,SRNew);
			*pSAD = sadx(16,16,pSC,srcStep,SRNew,16);
			break;
		}
	default:break;
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiSAD16x16_8u32s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiSAD16x16_8u32s(pSrc, srcStep, pRef, refStep, pSAD, mcType);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiSAD16x8_8u32s_C1R_c(
	const Ipp8u  *pSrcCur,  int  srcCurStep,  const Ipp8u  *pSrcRef,
	int srcRefStep, Ipp32s *pDst, Ipp32s mcType)
{
	/*NOTICE:
	The function ippiSAD16x8_8u32s_C1R is not really used in this encoder, the detail as following:

	this function is called by
	1, function ippVideoEncoderMPEG4::ME_SAD_16x8(mp4_Data_ME *meData) which in file: ippvideoencodermpeg4_me.cpp
	line 618# & line 633#

	2, function ippVideoEncoderMPEG4::ME_MacroBlock_P(mp4_Data_ME *meData)
	line 885# & line 886#
	3, ippVideoEncoderMPEG4::ME_MacroBlock_S(mp4_Data_ME *meData)
	line 1010# & line 1011#
	but in 2, 3 the four lines are commented in the sample.

	For above item 1, the fucntion ME_SAD_16x8 is called by
	a, function ippVideoEncoderMPEG4::ME_MacroBlock_P(mp4_Data_ME *meData)
	line 887# & line 888#
	b, function ippVideoEncoderMPEG4::ME_MacroBlock_S(mp4_Data_ME *meData)
	line 1012# & line 1013#

	But the four lines are also commented in the sample, so the ippiSAD16x8_8u32s_C1R is not used in the encoding processing.
	*/
#if 0

	Ipp8u *pSC, *pSR;
	if (!pSrcCur || ! pSrcRef)
		return ippStsNullPtrErr;

	pSC = pSrcCur;
	pSR = pSrcRef;
	switch(mcType)
	{
	case IPPVC_MC_APX_FF:	/* 0,	dst += abs(cur - ref) */
		*pDst = sadx(16,8,pSC,srcCurStep,pSR,srcRefStep);
		break;
	case IPPVC_MC_APX_FH:	/* 4, */
		{
			Ipp8u SRNew[128];
			mc_apx_fh(16,8,pSR,srcRefStep,SRNew);
			*pDst = sadx(16,8,pSC,srcCurStep,SRNew,16);
			break;
		}
	case IPPVC_MC_APX_HF:	/* 8, */
		{
			Ipp8u SRNew[128];
			mc_apx_hf(16,8,pSR,srcRefStep,SRNew);
			*pDst = sadx(16,8,pSC,srcCurStep,SRNew,16);
			break;
		}
	case IPPVC_MC_APX_HH:	/* 12, */
		{
			Ipp8u SRNew[128];
			mc_apx_hh(16,8,pSR,srcRefStep,SRNew);
			*pDst = sadx(16,8,pSC,srcCurStep,SRNew,16);
			break;
		}
	default:break;
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiSAD16x8_8u32s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiSAD16x8_8u32s_C1R(pSrcCur, srcCurStep,pSrcRef, srcRefStep, pDst, mcType);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiSAD8x8_8u32s_C1R_c(
	const Ipp8u*  pSrcCur, int srcCurStep, const Ipp8u*  pSrcRef,
	int     srcRefStep, Ipp32s* pDst,Ipp32s  mcType)
{
#if 1	

	Ipp8u *pSC, *pSR;
	if (!pSrcCur || ! pSrcRef)
		return ippStsNullPtrErr;

	pSC = pSrcCur;
	pSR = pSrcRef;
	switch(mcType)
	{
	case IPPVC_MC_APX_FF:	/* 0,	dst += abs(cur - ref) */
		*pDst = sadx(8,8,pSC,srcCurStep,pSR,srcRefStep);
		break;
	case IPPVC_MC_APX_FH:	/* 4, */
		{
			Ipp8u SRNew[64];
			mc_apx_fh(8,8,pSR,srcRefStep,SRNew);
			*pDst = sadx(8,8,pSC,srcCurStep,SRNew,8);
			break;
		}
	case IPPVC_MC_APX_HF:	/* 8, */
		{
			Ipp8u SRNew[64];
			mc_apx_hf(8,8,pSR,srcRefStep,SRNew);
			*pDst = sadx(8,8,pSC,srcCurStep,SRNew,8);
			break;
		}
	case IPPVC_MC_APX_HH:	/* 12, */
		{
			Ipp8u SRNew[64];
			mc_apx_hh(8,8,pSR,srcRefStep,SRNew);
			*pDst = sadx(8,8,pSC,srcCurStep,SRNew,8);
			break;
		}
	default:break;
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiSAD8x8_8u32s_C1R",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiSAD8x8_8u32s_C1R(pSrcCur, srcCurStep, pSrcRef,srcRefStep, pDst, mcType);
		return sts;
	}
#endif

	return ippStsNoErr;
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
