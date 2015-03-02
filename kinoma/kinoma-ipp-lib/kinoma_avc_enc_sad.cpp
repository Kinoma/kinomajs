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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef __KINOMA_IPP__
#include "ippvc.h"
#include "kinoma_avc_defines.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"

/// COMM :The following functions are some commam function in IPP:we need deleted it carefully
IppStatus  (__STDCALL *ippiSAD16x8_8u32s_C1R_universal) 							(const Ipp8u*  pSrcCur, int    srcCurStep,	const Ipp8u*  pSrcRef,	int		srcRefStep, Ipp32s *pDst,	Ipp32s mcType)=NULL;
IppStatus  (__STDCALL *ippiSAD16x16_8u32s_universal)								(const Ipp8u*  pSrc,	Ipp32s srcStep,		const Ipp8u*  pRef,		Ipp32s  refStep,	Ipp32s* pSAD,	Ipp32s mcType)=NULL;
IppStatus  (__STDCALL *ippiSAD8x8_8u32s_C1R_universal)								(const Ipp8u*  pSrcCur, int    srcCurStep,	const Ipp8u*  pSrcRef,	int		srcRefStep, Ipp32s* pDst,	Ipp32s mcType)=NULL;
IppStatus  (__STDCALL *ippiSAD4x4_8u32s_universal)									(const Ipp8u*  pSrc,	Ipp32s srcStep,		const Ipp8u*  pRef,		Ipp32s  refStep,	Ipp32s* pSAD,	Ipp32s mcType )=NULL;
IppStatus  (__STDCALL *ippiSAD16x16Blocks8x8_8u16u_universal)						(const Ipp8u*  pSrc,	Ipp32s srcStep,		const Ipp8u*  pRef,		Ipp32s  refStep,	Ipp16u* pDstSAD,Ipp32s mcType )=NULL;
IppStatus  (__STDCALL *ippiSAD16x16Blocks4x4_8u16u_universal) 						(const Ipp8u*  pSrc,	Ipp32s srcStep,		const Ipp8u*  pRef,		Ipp32s  refStep,	Ipp16u* pDstSAD,Ipp32s mcType)=NULL;
IppStatus  (__STDCALL *ippiSumsDiff16x16Blocks4x4_8u16s_C1_universal)				(const Ipp8u*  pSrc,	Ipp32s srcStep,		const Ipp8u*  pPred,	Ipp32s	predStep,	Ipp16s* pSums,	Ipp16s* pDiff)=NULL;
IppStatus  (__STDCALL *ippiSumsDiff8x8Blocks4x4_8u16s_C1_universal)					(const Ipp8u*  pSrc,	Ipp32s srcStep,		const Ipp8u*  pPred,	Ipp32s	predStep,	Ipp16s* pSums,	Ipp16s* pDiff)=NULL;
IppStatus  (__STDCALL *ippiGetDiff4x4_8u16s_C1_universal)							(const Ipp8u*  pSrcCur, Ipp32s srcCurStep,	const Ipp8u*  pSrcRef,	Ipp32s  srcRefStep, Ipp16s* pDstDiff, Ipp32s  dstDiffStep, Ipp16s* pDstPredictor, Ipp32s  dstPredictorStep, Ipp32s  mcType, Ipp32s  roundControl)=NULL;
IppStatus  (__STDCALL *ippiSub4x4_8u16s_C1R_universal)								(const Ipp8u*  pSrcCur, Ipp32s srcCurStep,	const Ipp8u*  pSrcRef,	Ipp32s  srcRefStep, Ipp16s* pDstDiff, Ipp32s  dstDiffStep )=NULL;
IppStatus  (__STDCALL *ippiEdgesDetect16x16_8u_C1R_universal)						(const Ipp8u*  pSrc,	Ipp32u srcStep,		Ipp8u EdgePelDifference,Ipp8u EdgePelCount, Ipp8u * pRes)=NULL;

#define  SAD(tmpResult0, pSrc0, pRef0 ) \
		tmpResult0 += absm(*(pSrc0)     - *(pRef0));     \
		tmpResult0 += absm(*((pSrc0)+1) - *((pRef0)+1)); \
		tmpResult0 += absm(*((pSrc0)+2) - *((pRef0)+2)); \
		tmpResult0 += absm(*((pSrc0)+3) - *((pRef0)+3)); \
		tmpResult0 += absm(*((pSrc0)+4) - *((pRef0)+4)); \
		tmpResult0 += absm(*((pSrc0)+5) - *((pRef0)+5)); \
		tmpResult0 += absm(*((pSrc0)+6) - *((pRef0)+6)); \
		tmpResult0 += absm(*((pSrc0)+7) - *((pRef0)+7)); 

#define  SAD4(tmpResult0, pSrc0, pRef0 ) \
		tmpResult0 += absm(*(pSrc0)     - *(pRef0));     \
		tmpResult0 += absm(*((pSrc0)+1) - *((pRef0)+1)); \
		tmpResult0 += absm(*((pSrc0)+2) - *((pRef0)+2)); \
		tmpResult0 += absm(*((pSrc0)+3) - *((pRef0)+3));

#define SUMD(pDstDiff0, pSrcCur0, pSrcRef0) \
		*(pDstDiff0  )  = *(pSrcCur0  ) - *(pSrcRef0  );\
		*(pDstDiff0+1)  = *(pSrcCur0+1) - *(pSrcRef0+1);\
		*(pDstDiff0+2)  = *(pSrcCur0+2) - *(pSrcRef0+2);\
		*(pDstDiff0+3)  = *(pSrcCur0+3) - *(pSrcRef0+3);



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
#if 1	//***bnie: need to turn this on for IPP 6.0

	Ipp8u *pSC, *pSR;
	if (!pSrcCur || ! pSrcRef)
		return ippStsNullPtrErr;

	pSC = (Ipp8u  *)pSrcCur;
	pSR = (Ipp8u  *)pSrcRef;
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



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEdgesDetect16x16_8u_C1R_c
//
//  Purpose:
//  This function detects edges inside 16x16 block:
//      finds pair of neighboring (horizontal and vertical) elements with
//  difference is greater than EdgePelDifference.
//  In the case of number of pairs is greater than EdgePelCount,
//  edges are detected and flag (* pRes) is set to 1.
//  Otherwise, edges aren't detected  ((* pRes) is set to 0)
//
//
//  Parameters:
//      pSrc                    Pointer to 16x16 block in current plan
//      srcStep                 Step of the current plan, specifying width of the plane in bytes.
//      EdgePelDifference       The value for estimation of difference between neighboring elements.
//      EdgePelCount            The value for estimation of number of pairs with "big difference"
//      pRes                    Pointers to output value. (*pRes) is equal 1 in
//                              the case of edges are detected and it is equal
//                              0 in the case of edges aren't detected. I
//
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  one of the input pointers is NULL
*/
// Please see Page 1131 for details: But the documents lost abs operation really. -- WWD in 2006-08-01

//***bnie***
//***bnie: armv6 optimization???
IppStatus __STDCALL ippiEdgesDetect16x16_8u_C1R_c(
        const Ipp8u *pSrc,
        Ipp32u srcStep,
        Ipp8u EdgePelDifference,
        Ipp8u EdgePelCount,
        Ipp8u   *pRes)
{
	int		col, row;
	int		count = 0;

	const Ipp8u *pSrcNext, *pSrcCurr, *pSrcNext2;

	// Set default value
	*pRes = 0;

	pSrcCurr = pSrc;
	for(row=0; row<15; row++)
	{		
		pSrcNext  = pSrcCurr + 1;
		pSrcNext2 = pSrcCurr + srcStep;;
		for(col =0; col<15; col++)
		{
			if(absm((*(pSrcCurr+col)) - (*(pSrcNext+col))) > EdgePelDifference)
				count += 1;

			if(absm((*(pSrcCurr+col)) - (*(pSrcNext2+col))) > EdgePelDifference)
				count += 1;
		}

		pSrcCurr += srcStep ;
	}

	if(count > EdgePelCount)
		*pRes = 1; 

	return ippStsNoErr;
}

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSAD16x16_8u32s
//
//  Purpose:
//    Evaluates the sum of absolute difference (SAD) between all elements of
//    the current block and the corresponding elements of the reference block.
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  One of the pointers is NULL
//    ippStsStepErr     srcCurStep or srcRefStep is less than or equal to 0
//
//  Parameters:
//    pSrc           Pointer to the current block
//    srcStep        Step in bytes through the current block
//    pRef           Pointer to the reference block
//    refStep        Step in bytes through the reference block
//    mcType         Type of motion compensation
//    pSAD           Pointer to the result
*/

//***bnie: arm v6 optimization exists
IppStatus  __STDCALL  ippiSAD16x16_8u32s_00_c (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRef,
        Ipp32s  refStep,
        Ipp32s* pSAD)
{
	int	j;
	int	tmpResult;	

	tmpResult = 0;
	for(j=16; j>0; j--)
	{
		SAD(tmpResult, pSrc,  pRef);
		SAD(tmpResult, (pSrc+8),  (pRef+8));
		pSrc += srcStep;
		pRef += refStep;
	}

	*pSAD = tmpResult;

	return ippStsNoErr;
}


static void __inline avg16x16_fh(const Ipp8u *s0, int srb, Ipp8u *d)
{
	int i,j;
	unsigned char  *s  = (Ipp8u *)s0;
	unsigned char  *ss = s + srb;

	for (j = 0; j < 16; j++)
	{
		int s0  = s[ 0];
		int ss0 = ss[0];
		int s1  = s[ 1];
		int ss1 = ss[1];
		int s2  = s[ 2];
		int ss2 = ss[2];
		int s3  = s[ 3];
		int ss3 = ss[3];
		int t, dd;

		dd = (s0  + ss0  + 1) >> 1;
		int s4  = s[ 4];
		int ss4 = ss[4];
		t = (s1  + ss1  + 1) >> 1;
		dd = dd|(t<<8);
		int s5  = s[ 5];
		int ss5 = ss[5];
		t = (s2  + ss2  + 1) >> 1;
		dd = dd|(t<<16);
		int s6  = s[ 6];
		int ss6 = ss[6];
		t = (s3  + ss3  + 1) >> 1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;

		int s7  = s[ 7];
		int ss7 = ss[7];
		dd = (s4  + ss4  + 1) >> 1;
		int s8  = s[ 8];
		int ss8 = ss[8];
		t = (s5  + ss5  + 1) >> 1;
		dd = dd|(t<<8);
		int s9  = s[ 9];
		int ss9 = ss[9];
		t = (s6  + ss6  + 1) >> 1;
		dd = dd|(t<<16);
		int s10 = s[10];
		int ss10 = ss[10];
		t = (s7  + ss7  + 1) >> 1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;

		int s11  = s[11];
		int ss11 = ss[11];
		dd = (s8  + ss8  + 1) >> 1;
		int s12  = s[12];
		int ss12 = ss[12];
		t = (s9  + ss9  + 1) >> 1;
		dd = dd|(t<<8);
		int s13  = s[13];
		int ss13 = ss[13];
		t = (s10 + ss10 + 1) >> 1;
		dd = dd|(t<<16);
		int s14  = s[14];
		int ss14 = ss[14];
		t = (s11 + ss11 + 1) >> 1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;
		
		int s15  = s[15];
		int ss15 = ss[15];
		dd = (s12 + ss12 + 1) >> 1;
		t = (s13 + ss13 + 1) >> 1;
		dd = dd|(t<<8);
		t = (s14 + ss14 + 1) >> 1;
		dd = dd|(t<<16);
		t = (s15 + ss15 + 1) >> 1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;

		s  += srb;
		ss += srb;
	}
}

static void __inline avg16x16_hf(const Ipp8u *s, int srb, Ipp8u *d)
{
	int i,j;

	for(j=0; j<16; j++)
	{
		int s0 = s[0];
		int s1 = s[1];
		int s2 = s[2];
		int s3 = s[3];
		int s4 = s[4];
		int t, dd;

		dd = (s0 + s1 +1)>>1;
		int s5 = s[5];
		t = (s1 + s2 +1)>>1;
		dd = dd|(t<<8);
		int s6 = s[6];
		t = (s2 + s3 +1)>>1;
		dd = dd|(t<<16);
		int s7 = s[7];
		t = (s3 + s4 +1)>>1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;

		int s8 = s[8];
		dd = (s4 + s5 +1)>>1;
		int s9 = s[9];
		t = (s5 + s6 +1)>>1;
		dd = dd|(t<<8);
		int s10 = s[10];
		t = (s6 + s7 +1)>>1;
		dd = dd|(t<<16);
		int s11 = s[11];
		t = (s7 + s8 +1)>>1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;

		int s12 = s[12];
		dd = ( s8 +  s9 +1)>>1;
		int s13 = s[13];
		t = ( s9 + s10 +1)>>1;
		dd = dd|(t<<8);
		int s14 = s[14];
		t = (s10 + s11 +1)>>1;
		dd = dd|(t<<16);
		int s15 = s[15];
		t = (s11 + s12 +1)>>1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;

		int s16 = s[16];
		dd = (s12 + s13 +1)>>1;
		t = (s13 + s14 +1)>>1;
		dd = dd|(t<<8);
		t = (s14 + s15 +1)>>1;
		dd = dd|(t<<16);
		t = (s15 + s16 +1)>>1;
		dd = dd|(t<<24);
		*((long *)d) = dd; d+= 4;
		
		s += srb;
	}
}


static void __inline avg16x16_hh(const  Ipp8u *s, int srb, Ipp8u *d)
{
	int i,j;
	Ipp8u  *ss = (Ipp8u *)s + srb;

	for(j=0; j<16; j++)
	{
		int s0  = s[ 0];
		int ss0 = ss[ 0];
		int s1  = s[ 1];
		int ss1 = ss[ 1];
		int s2  = s[ 2];
		int ss2 = ss[ 2];
		int s3  = s[ 3];
		int ss3 = ss[ 3];

		*d++ = (s0 + s1 + ss0 + ss1 +2)>>2;
		int s4   =  s[ 4];
		int ss4  = ss[ 4];
		*d++ = (s1 + s2 + ss1 + ss2 +2)>>2;
		int s5   =  s[ 5];
		int ss5  = ss[ 5];
		*d++ = (s2 + s3 + ss2 + ss3 +2)>>2;
		int s6   =  s[ 6];
		int ss6  = ss[ 6];
		*d++ = (s3 + s4 + ss3 + ss4 +2)>>2;
		int s7   =  s[ 7];
		int ss7  = ss[ 7];
		*d++ = (s4 + s5 + ss4 + ss5 +2)>>2;
		int s8   =  s[ 8];
		int ss8  = ss[ 8];
		*d++ = (s5 + s6 + ss5 + ss6 +2)>>2;
		int s9   =  s[ 9];
		int ss9  = ss[ 9];
		*d++ = (s6 + s7 + ss6 + ss7 +2)>>2;
		int s10   =  s[10];
		int ss10  = ss[10];
		*d++ = (s7 + s8 + ss7 + ss8 +2)>>2;
		int s11   =  s[11];
		int ss11  = ss[11];
		*d++ = (s8 + s9 + ss8 + ss9 +2)>>2;
		int s12   =  s[12];
		int ss12  = ss[12];
		*d++ = (s9 + s10 + ss9 + ss10 +2)>>2;
		int s13   =  s[13];
		int ss13  = ss[13];
		*d++ = (s10 + s11 + ss10 + ss11 +2)>>2;
		int s14   =  s[14];
		int ss14  = ss[14];
		*d++ = (s11 + s12 + ss11 + ss12 +2)>>2;
		int s15   =  s[15];
		int ss15  = ss[15];
		*d++ = (s12 + s13 + ss12 + ss13 +2)>>2;
		int s16   =  s[16];
		int ss16  = ss[16];
		*d++ = (s13 + s14 + ss13 + ss14 +2)>>2;
		*d++ = (s14 + s15 + ss14 + ss15 +2)>>2;
		*d++ = (s15 + s16 + ss15 + ss16 +2)>>2;
		
		s  += srb;
		ss += srb;
	}
}

#ifdef __KINOMA_IPP_ARM_V6__

IppStatus  __STDCALL  ippiSAD16x16_8u32s_generic_arm_v6 (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRef,
        Ipp32s  refStep,
        Ipp32s* pSAD,
        Ipp32s  mcType)
{
	__ALIGN4(Ipp8u, avg, 256);

	if(mcType ==IPPVC_MC_APX_FF)
		return ippiSAD16x16_8u32s_misaligned_arm_v6(pSrc,srcStep,pRef,refStep,pSAD, mcType);
	else if( mcType == IPPVC_MC_APX_FH )	//vertical average
		avg16x16_fh(pRef, refStep, avg);
	else if( mcType == IPPVC_MC_APX_HF )	//horizontal average
		avg16x16_hf(pRef, refStep, avg);
	else if( mcType == IPPVC_MC_APX_HH )	//middle average
		avg16x16_hh(pRef, refStep, avg);

	return ippiSAD16x16_8u32s_aligned_4_arm_v6(pSrc,srcStep,avg,pSAD);
}

#endif


IppStatus  __STDCALL  ippiSAD16x16_8u32s_c (
  const Ipp8u*  pSrc,
        Ipp32s  srcStep,
  const Ipp8u*  pRef,
        Ipp32s  refStep,
        Ipp32s* pSAD,
        Ipp32s  mcType)
{
	__ALIGN4(Ipp8u, avg, 256);
	Ipp8u *d = avg;
	Ipp8u  *s = (Ipp8u *)pRef;

	if(mcType ==IPPVC_MC_APX_FF)
		return ippiSAD16x16_8u32s_00_c(pSrc,srcStep,pRef,refStep,pSAD);
	else if( mcType == IPPVC_MC_APX_FH )			//vertical average
		avg16x16_fh(s, refStep, d);
	else if( mcType == IPPVC_MC_APX_HF )	//horizontal average
		avg16x16_hf(s, refStep, d);
	else if( mcType == IPPVC_MC_APX_HH )	//middle average
		avg16x16_hh(s, refStep, d);

	return ippiSAD16x16_8u32s_00_c(pSrc,srcStep,avg,16,pSAD);
}


//***bnie: arm v6 optimization exists
IppStatus  __STDCALL  ippiSAD8x8_8u32s_C1R_c(
  const Ipp8u*  pSrcCur,
        int     srcCurStep,
  const Ipp8u*  pSrcRef,
        int     srcRefStep,
        Ipp32s* pDst,
        Ipp32s  mcType)
{
	int	j;
	int	tmpResult;	

	// This is only one Limite Version of this function : Used for AVC ONLY!!!!!!!!!!
	// Do NOT support MPEG2	
	assert(mcType == 0);

	//case IPPVC_MC_APX_FF:  // 
	tmpResult = 0;
	for(j=8; j>0; j--)
	{
		SAD(tmpResult, (pSrcCur),  (pSrcRef));	
		pSrcCur += srcCurStep;
		pSrcRef += srcRefStep;
	}

	*pDst = tmpResult;

	return ippStsNoErr;
}


//***bnie£ºarm v6 opt exists¡¡
IppStatus  __STDCALL  ippiSAD4x4_8u32s_c(const Ipp8u*  pSrc,
                                    Ipp32s        srcStep,
                                    const Ipp8u*  pRef,
                                    Ipp32s        refStep,
                                    Ipp32s*       pSAD,
                                    Ipp32s        mcType )
{
	int	j;
	int	tmpResult;	

	// This is only one Limite Version of this function : Used for AVC ONLY!!!!!!!!!!
	// Do NOT support MPEG2	
	assert(mcType == 0);

	//case IPPVC_MC_APX_FF:  // 
	tmpResult = 0;
	for(j=4; j>0; j--)
	{
		SAD4(tmpResult, (pSrc),  (pRef));
		pSrc += srcStep;
		pRef += refStep;
	}

	*pSAD = tmpResult;
	return ippStsNoErr;
}

IppStatus  __STDCALL  ippiSAD16x16Blocks8x8_8u16u_c(const   Ipp8u*  pSrc,
                                               Ipp32s  srcStep,
                                               const   Ipp8u*  pRef,
                                               Ipp32s  refStep,
                                               Ipp16u*  pDstSAD,
                                              Ipp32s   mcType )
{
	int	j, k;
	int	tmpResult, tmpResult2;	

	// This is only one Limite Version of this function : Used for AVC ONLY!!!!!!!!!!
	// Do NOT support MPEG2	
	assert(mcType == 0);

	for(k=0; k<3; k+=2)
	{
		//case IPPVC_MC_APX_FF:  // 
		tmpResult  = 0;
		tmpResult2 = 0;
		for(j=0; j<8; j++)
		{
			SAD(tmpResult, (pSrc),  (pRef));
			SAD(tmpResult2, (pSrc +  8),  (pRef +  8));
			pSrc += srcStep;
			pRef += refStep;
		}


		*(pDstSAD+k)   = tmpResult;
		*(pDstSAD+k+1) = tmpResult2;
	}

	return ippStsNoErr;
} 

IppStatus  __STDCALL  ippiSAD16x16Blocks4x4_8u16u_c (const   Ipp8u*  pSrc,
                                                Ipp32s  srcStep,
                                                const   Ipp8u*  pRef,
                                                Ipp32s  refStep,
                                                Ipp16u*  pDstSAD,
                                                Ipp32s   mcType)

{
	int	j, k;
	int	tmpResult[4];	

	// This is only one Limite Version of this function : Used for AVC ONLY!!!!!!!!!!
	// Do NOT support MPEG2	
	assert(mcType == 0);

	for(k=0; k<13; k+=4)
	{
		//case IPPVC_MC_APX_FF:  // 
		tmpResult[0] = tmpResult[1] = tmpResult[2] = tmpResult[3] = 0;

		for(j=0; j<8; j++)
		{
			SAD4(tmpResult[0], (pSrc     ),  (pRef    ));
			SAD4(tmpResult[1], (pSrc +  4),  (pRef + 4));
			SAD4(tmpResult[2], (pSrc +  8),  (pRef +  8));
			SAD4(tmpResult[3], (pSrc + 12),  (pRef + 12));

			pSrc += srcStep;
			pRef += refStep;
		}

		*(pDstSAD+k)   = tmpResult[0];
		*(pDstSAD+k+1) = tmpResult[1];
		*(pDstSAD+k+2) = tmpResult[2];
		*(pDstSAD+k+3) = tmpResult[3];
	}

	return ippStsNoErr;
}

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiSumsDiff16x16Blocks4x4_8u16s_C1, ippiSumsDiff16x16Blocks4x4_8u16s_C1
//
//  Purpose:
//      These functions evaluates difference between current and reference 4x4 blocks
//      and calculates sums of 4x4 residual blocks elements
//  Parameters:
//      pSrc    Pointer  block in current plane
//      srcStep Step of the current plane, specifying width of the plane in bytes.
//      pPred   Pointer to  reference block
//  predStep Step of the reference plane, specifying width of the plane in bytes.
//      pDiff   If it isn't zero, pointer to array  that contains a sequence of 4x4
//      residual blocks.  The array's filled by function if pDiff isn't null.
//      pSums   Pointer to array that contains sums of 4x4 difference blocks coefficients.
//      The array's filled by function.
//
//  Returns:
//    ippStsNoErr       No error
//    ippStsNullPtrErr  one of the input pointers is NULL
*/

//***bnie: need to implement!!!
IppStatus  __STDCALL  ippiSumsDiff16x16Blocks4x4_8u16s_C1_c
 (
        const Ipp8u*          pSrc,
        Ipp32s          srcStep,
        const Ipp8u*          pPred,
        Ipp32s          predStep,
        Ipp16s* pSums,
        Ipp16s* pDiff
)
{
	// Please Notes: The declation of this functions exsit error in ippi.pdf
	int		k1, k2;
	int		tmpResult[4];

	for(k1=0; k1<4; k1++)		// Vertical 4 line
	{
		tmpResult[0] = tmpResult[1] = tmpResult[2] = tmpResult[3] = 0;

		for(k2=0; k2<4; k2++)	// Hortizental 4 clolums
		{
			// index 0
			SUMD((pDiff), (pSrc), (pPred) );
			tmpResult[0] += *(pDiff) + *(pDiff+1 ) + *(pDiff+2 ) + *(pDiff+3 );

			// index 1
			SUMD((pDiff+16), (pSrc+4), (pPred+4) );
			tmpResult[1] += *(pDiff+16) + *(pDiff+17) + *(pDiff+18) + *(pDiff+19);

			// index 2
			SUMD((pDiff+32), (pSrc+8), (pPred+8) );
			tmpResult[2] += *(pDiff+32) + *(pDiff+33) + *(pDiff+34) + *(pDiff+35);

			// index 3
			SUMD((pDiff+48), (pSrc+12), (pPred+12) );
			tmpResult[3] += *(pDiff+48) + *(pDiff+49) + *(pDiff+50) + *(pDiff+51);

			pSrc  += srcStep;
			pPred += predStep;

			pDiff += 4;
		}

		pDiff += 48;

		// Save Result
		*(pSums+0) = tmpResult[0];
		*(pSums+1) = tmpResult[1];
		*(pSums+2) = tmpResult[2];
		*(pSums+3) = tmpResult[3];
		pSums+= 4;
	}

	return ippStsNoErr;
}

//***bnie: need to implement!!!
IppStatus  __STDCALL ippiSumsDiff8x8Blocks4x4_8u16s_C1_c(
        const Ipp8u*    pSrc,
        Ipp32s          srcStep,
        const Ipp8u*    pPred,
        Ipp32s          predStep,
        Ipp16s*			pSums,
        Ipp16s*			pDiff
)
{
	// Please Notes: The declation of this functions exsit error in ippi.pdf
	int		k1, k2;
	int		tmpResult[2];

	for(k1=0; k1<2; k1++)		// Vertical 4 4x4 blocks: process 2 4x4 block each loop -- in H direction
	{
		tmpResult[0] = tmpResult[1] = 0;

		for(k2=0; k2<4; k2++)	// Hortizental 4 clolums
		{
			// index 0
			SUMD((pDiff), (pSrc), (pPred) );
			tmpResult[0] += *(pDiff) + *(pDiff+1 ) + *(pDiff+2 ) + *(pDiff+3 );

			// index 1
			SUMD((pDiff+16), (pSrc+4), (pPred+4) );
			tmpResult[1] += *(pDiff+16) + *(pDiff+17) + *(pDiff+18) + *(pDiff+19);

			pSrc  += srcStep;
			pPred += predStep;
			pDiff += 4;
		}

		pDiff += 16;

		// Save Result
		*(pSums+0) = tmpResult[0];
		*(pSums+1) = tmpResult[1];
		pSums+= 2;
	}

	return ippStsNoErr;
}

//***bnie: need to implement!!!
 IppStatus __STDCALL ippiGetDiff4x4_8u16s_C1_c(
  const Ipp8u*  pSrcCur,
        Ipp32s  srcCurStep,
  const Ipp8u*  pSrcRef,
                Ipp32s  srcRefStep,
                Ipp16s* pDstDiff,
                Ipp32s  dstDiffStep,

                Ipp16s* pDstPredictor,
                Ipp32s  dstPredictorStep,
                Ipp32s  mcType,
                Ipp32s  roundControl)
{
	// The last four parameters are reserved parameters, which must be set to ZERO!!
	// Please note: The unit for dstDiffStep is 8bits (byte),and for pDstDiff is 16 bits, so we use dstDiffStep as dstDiffStep >>1 -- WWD in 2006-07-31
	int		j;
	int		i = (dstDiffStep>>1);
	for(j=0; j<4; j++)
	{
		SUMD((pDstDiff), (pSrcCur), (pSrcRef) );

		pDstDiff = pDstDiff + i;
		pSrcCur += srcCurStep;
		pSrcRef += srcRefStep;
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippiSub4x4_8u16s_C1R_c
(	
	const Ipp8u* pSrc1,
	int			 src1Step,
	const Ipp8u* pSrc2,
	int			 src2Step,
	Ipp16s*		 pDst,
	int			 dstStep
)
{
	return ippiGetDiff4x4_8u16s_C1_c( pSrc1, src1Step, pSrc2, src2Step, pDst, dstStep, 0, 0, 0, 0 );
}

#endif
