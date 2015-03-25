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

IppStatus (__STDCALL *ippiOBMC8x8HP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep, Ipp8u* pDst,int dstStep,const IppMotionVector* pMVCur,const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow, int rounding)=NULL;
IppStatus (__STDCALL *ippiOBMC8x8QP_MPEG4_8u_C1R_universal)				(const Ipp8u* pSrc, int srcStep,Ipp8u* pDst,int dstStep, const IppMotionVector* pMVCur, const IppMotionVector* pMVLeft, const IppMotionVector* pMVRight,const IppMotionVector* pMVAbove, const IppMotionVector* pMVBelow,int rounding)=NULL;

/* OBMC_H tables Ipp8u*/
static int obmch0[64] =
{
    4,5,5,5,5,5,5,4,
    5,5,5,5,5,5,5,5,
    5,5,6,6,6,6,5,5,
    5,5,6,6,6,6,5,5,
    5,5,6,6,6,6,5,5,
    5,5,6,6,6,6,5,5,
    5,5,5,5,5,5,5,5,
    4,5,5,5,5,5,5,4
};

static int obmch1[64] = 
{
	2,2,2,2,2,2,2,2,
    1,1,2,2,2,2,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,2,2,2,2,1,1,
    2,2,2,2,2,2,2,2

};

static int obmch2[64] = 
{
    2,1,1,1,1,1,1,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,1,1,1,1,1,1,2
};
/* obmc tables end*/


/* 4x8 block 1/2 interpolation, used in OBMC HP */
void Copy4x8HP_8u_C1R(							
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding)
{
	int i;
	register int round = 1 - rounding;
	Ipp8u *pS, *pD;

	pS = (Ipp8u*)pSrc;
	pD = pDst;

	if (acc == 0)		/*the a points, every point need not to think the border of block*/
	{
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (i = 8; i > 0; i--)
		{
#ifdef WIN32
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*p1 = *p2;
#else
			pD[0] = pS[0];
			pD[1] = pS[1];
			pD[2] = pS[2];
			pD[3] = pS[3];
#endif
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 1)		/* the b points*/
	{
		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + round)>>1;
			pD[1] = (pS[1] + pS[2] + round)>>1;
			pD[2] = (pS[2] + pS[3] + round)>>1;
			pD[3] = (pS[3] + pS[4] + round)>>1;

			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 2)		/* the c points*/
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;

		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pSN[0] + round)>>1;
			pD[1] = (pS[1] + pSN[1] + round)>>1;
			pD[2] = (pS[2] + pSN[2] + round)>>1;
			pD[3] = (pS[3] + pSN[3] + round)>>1;

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}

	}
	else if (acc == 3)		/* the d points */
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;
		round = 2 - rounding;
		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + pSN[0] + pSN[1] + round)>>2;
			pD[1] = (pS[1] + pS[2] + pSN[1] + pSN[2] + round)>>2;
			pD[2] = (pS[2] + pS[3] + pSN[2] + pSN[3] + round)>>2;
			pD[3] = (pS[3] + pS[4] + pSN[3] + pSN[4] + round)>>2;

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}	
	}
	
}

/*
declared in ippvc.h
OBMC8x8HP_MPEG4,
OBMC8x8QP_MPEG4,
Perform overlapped block motion compensation
(OBMC) for one luminance block.
*/
/* copy from mp4dec.h #line 158 */
#define K_MP4_MV_ACC_HP(dx, dy) \
	((((dy) & 1) << 1) + ((dx) & 1))
IppStatus __STDCALL 
ippiOBMC8x8HP_MPEG4_8u_C1R_c(
							  const Ipp8u*              pSrc,
							  int                 srcStep,
							  Ipp8u*              pDst,
							  int                 dstStep,
							  const IppMotionVector*    pMVCur,
							  const IppMotionVector*    pMVLeft,
							  const IppMotionVector*    pMVRight,
							  const IppMotionVector*    pMVAbove,
							  const IppMotionVector*    pMVBelow,
							  int                 rounding)

{
	int i, cur_mvd, lft_mvd, rgt_mvd, abv_mvd, blw_mvd;
	int *pH0, *pH1, *pH2;
	Ipp8u *pS, *pD;
	Ipp8u IC[64], IL[32], IR[32], IA[32], IB[32];
	Ipp8u *pIC,*pIL,*pIR,*pIA,*pIB;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiOBMC8x8HP_MPEG4_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	cur_mvd = (pMVCur->dy>>1) * srcStep + (pMVCur->dx>>1);
	lft_mvd = (pMVLeft->dy>>1) * srcStep + (pMVLeft->dx>>1);		/* left half */
	rgt_mvd = (pMVRight->dy>>1) * srcStep + (pMVRight->dx>>1) + 4;	/* right half */
	abv_mvd = (pMVAbove->dy>>1) * srcStep + (pMVAbove->dx>>1);		/* top half */
	blw_mvd = ((pMVBelow->dy>>1) + 4) * (srcStep) + (pMVBelow->dx>>1);  /* bottom half */

	ippiCopy8x8HP_8u_C1R_c(pSrc+cur_mvd,srcStep,IC,8,K_MP4_MV_ACC_HP(pMVCur->dx,pMVCur->dy),rounding);

	Copy4x8HP_8u_C1R(pSrc+lft_mvd,srcStep,IL,4,K_MP4_MV_ACC_HP(pMVLeft->dx,pMVLeft->dy),rounding);
	Copy4x8HP_8u_C1R(pSrc+rgt_mvd,srcStep,IR,4,K_MP4_MV_ACC_HP(pMVRight->dx,pMVRight->dy),rounding);

	ippiCopy8x4HP_8u_C1R_c(pSrc+abv_mvd,srcStep,IA,8,K_MP4_MV_ACC_HP(pMVAbove->dx,pMVAbove->dy),rounding);
	ippiCopy8x4HP_8u_C1R_c(pSrc+blw_mvd,srcStep,IB,8,K_MP4_MV_ACC_HP(pMVBelow->dx,pMVBelow->dy),rounding);

	pS = (Ipp8u*)pSrc;
	pD = pDst;
	pH0 = obmch0;
	pH1 = obmch1;
	pH2 = obmch2;
	pIC = IC;
	pIL = IL;
	pIR = IR;
	pIA = IA;
	pIB = IB;

	for (i = 4; i > 0; i--)
	{
		pD[0] = (pIC[0] * pH0[0] + pIA[0] * pH1[0] + pIL[0] * pH2[0] + 4) >> 3;
		pD[1] = (pIC[1] * pH0[1] + pIA[1] * pH1[1] + pIL[1] * pH2[1] + 4) >> 3;
		pD[2] = (pIC[2] * pH0[2] + pIA[2] * pH1[2] + pIL[2] * pH2[2] + 4) >> 3;
		pD[3] = (pIC[3] * pH0[3] + pIA[3] * pH1[3] + pIL[3] * pH2[3] + 4) >> 3;

		pD[4] = (pIC[4] * pH0[4] + pIA[4] * pH1[4] + pIR[0] * pH2[4] + 4) >> 3;
		pD[5] = (pIC[5] * pH0[5] + pIA[5] * pH1[5] + pIR[1] * pH2[5] + 4) >> 3;
		pD[6] = (pIC[6] * pH0[6] + pIA[6] * pH1[6] + pIR[2] * pH2[6] + 4) >> 3;
		pD[7] = (pIC[7] * pH0[7] + pIA[7] * pH1[7] + pIR[3] * pH2[7] + 4) >> 3;
		pD += dstStep;
		pS += srcStep;
		pH0 += 8;
		pH1 += 8;
		pH2 += 8;
		pIC += 8;
		pIL += 4;
		pIR += 4;
		pIA += 8;
	}
	for (i = 4; i > 0; i--)
	{
		pD[0] = (pIC[0] * pH0[0] + pIB[0] * pH1[0] + pIL[0] * pH2[0] + 4) >> 3;
		pD[1] = (pIC[1] * pH0[1] + pIB[1] * pH1[1] + pIL[1] * pH2[1] + 4) >> 3;
		pD[2] = (pIC[2] * pH0[2] + pIB[2] * pH1[2] + pIL[2] * pH2[2] + 4) >> 3;
		pD[3] = (pIC[3] * pH0[3] + pIB[3] * pH1[3] + pIL[3] * pH2[3] + 4) >> 3;

		pD[4] = (pIC[4] * pH0[4] + pIB[4] * pH1[4] + pIR[0] * pH2[4] + 4) >> 3;
		pD[5] = (pIC[5] * pH0[5] + pIB[5] * pH1[5] + pIR[1] * pH2[5] + 4) >> 3;
		pD[6] = (pIC[6] * pH0[6] + pIB[6] * pH1[6] + pIR[2] * pH2[6] + 4) >> 3;
		pD[7] = (pIC[7] * pH0[7] + pIB[7] * pH1[7] + pIR[3] * pH2[7] + 4) >> 3;
		pD += dstStep;
		pS += srcStep;
		pH0 += 8;
		pH1 += 8;
		pH2 += 8;
		pIC += 8;
		pIL += 4;
		pIR += 4;
		pIB += 8;
	}

	return ippStsNoErr;
}


#define K_MP4_MV_ACC_QP(dx, dy) \
	((((dy) & 3) << 2) + ((dx) & 3))
IppStatus __STDCALL 
ippiOBMC8x8QP_MPEG4_8u_C1R_c(
							  const Ipp8u*              pSrc,
							  int                 srcStep,
							  Ipp8u*              pDst,
							  int                 dstStep,
							  const IppMotionVector*    pMVCur,
							  const IppMotionVector*    pMVLeft,
							  const IppMotionVector*    pMVRight,
							  const IppMotionVector*    pMVAbove,
							  const IppMotionVector*    pMVBelow,
							  int                 rounding)
{
	int i, cur_mvd, lft_mvd, rgt_mvd, abv_mvd, blw_mvd;
	int *pH0, *pH1, *pH2;
	Ipp8u *pS, *pD;
	Ipp8u IC[64], IL[64], IR[64], IA[64], IB[64];
	Ipp8u *pIC,*pIL,*pIR,*pIA,*pIB;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiOBMC8x8QP_MPEG4_8u_C1R		with rounding(0,1): ",rounding);
	}
#endif

	cur_mvd = (pMVCur->dy>>2) * srcStep + (pMVCur->dx>>2);
	lft_mvd = (pMVLeft->dy>>2) * srcStep + (pMVLeft->dx>>2);		
	rgt_mvd = (pMVRight->dy>>2) * srcStep + (pMVRight->dx>>2);		
	abv_mvd = (pMVAbove->dy>>2) * srcStep + (pMVAbove->dx>>2);		
	blw_mvd = ((pMVBelow->dy>>2)) * (srcStep) + (pMVBelow->dx>>2);  

	ippiCopy8x8QP_MPEG4_8u_C1R_c(pSrc+cur_mvd,srcStep,IC,8,K_MP4_MV_ACC_QP(pMVCur->dx,pMVCur->dy),rounding);

	/* for the feature of QP interpolation, even if we just use 4x8 and 8x4,we must interpolate 8x8 here*/
	ippiCopy8x8QP_MPEG4_8u_C1R_c(pSrc+lft_mvd,srcStep,IL,8,K_MP4_MV_ACC_QP(pMVLeft->dx,pMVLeft->dy),rounding);
	ippiCopy8x8QP_MPEG4_8u_C1R_c(pSrc+rgt_mvd,srcStep,IR,8,K_MP4_MV_ACC_QP(pMVRight->dx,pMVRight->dy),rounding);

	ippiCopy8x8QP_MPEG4_8u_C1R_c(pSrc+abv_mvd,srcStep,IA,8,K_MP4_MV_ACC_QP(pMVAbove->dx,pMVAbove->dy),rounding);
	ippiCopy8x8QP_MPEG4_8u_C1R_c(pSrc+blw_mvd,srcStep,IB,8,K_MP4_MV_ACC_QP(pMVBelow->dx,pMVBelow->dy),rounding);

	pS = (Ipp8u*)pSrc;
	pD = pDst;
	pH0 = obmch0;
	pH1 = obmch1;
	pH2 = obmch2;
	pIC = IC;
	pIL = IL;
	pIR = IR;
	pIA = IA;
	pIB = IB;

	for (i = 4; i > 0; i--)
	{
		pD[0] = (pIC[0] * pH0[0] + pIA[0] * pH1[0] + pIL[0] * pH2[0] + 4) >> 3;
		pD[1] = (pIC[1] * pH0[1] + pIA[1] * pH1[1] + pIL[1] * pH2[1] + 4) >> 3;
		pD[2] = (pIC[2] * pH0[2] + pIA[2] * pH1[2] + pIL[2] * pH2[2] + 4) >> 3;
		pD[3] = (pIC[3] * pH0[3] + pIA[3] * pH1[3] + pIL[3] * pH2[3] + 4) >> 3;

		pD[4] = (pIC[4] * pH0[4] + pIA[4] * pH1[4] + pIR[4] * pH2[4] + 4) >> 3;
		pD[5] = (pIC[5] * pH0[5] + pIA[5] * pH1[5] + pIR[5] * pH2[5] + 4) >> 3;
		pD[6] = (pIC[6] * pH0[6] + pIA[6] * pH1[6] + pIR[6] * pH2[6] + 4) >> 3;
		pD[7] = (pIC[7] * pH0[7] + pIA[7] * pH1[7] + pIR[7] * pH2[7] + 4) >> 3;
		pD += dstStep;
		pS += srcStep;
		pH0 += 8;
		pH1 += 8;
		pH2 += 8;
		pIC += 8;
		pIL += 8;
		pIR += 8;
		pIA += 8;
	}
	pIB += 32;
	for (i = 4; i > 0; i--)
	{
		pD[0] = (pIC[0] * pH0[0] + pIB[0] * pH1[0] + pIL[0] * pH2[0] + 4) >> 3;
		pD[1] = (pIC[1] * pH0[1] + pIB[1] * pH1[1] + pIL[1] * pH2[1] + 4) >> 3;
		pD[2] = (pIC[2] * pH0[2] + pIB[2] * pH1[2] + pIL[2] * pH2[2] + 4) >> 3;
		pD[3] = (pIC[3] * pH0[3] + pIB[3] * pH1[3] + pIL[3] * pH2[3] + 4) >> 3;

		pD[4] = (pIC[4] * pH0[4] + pIB[4] * pH1[4] + pIR[4] * pH2[4] + 4) >> 3;
		pD[5] = (pIC[5] * pH0[5] + pIB[5] * pH1[5] + pIR[5] * pH2[5] + 4) >> 3;
		pD[6] = (pIC[6] * pH0[6] + pIB[6] * pH1[6] + pIR[6] * pH2[6] + 4) >> 3;
		pD[7] = (pIC[7] * pH0[7] + pIB[7] * pH1[7] + pIR[7] * pH2[7] + 4) >> 3;
		pD += dstStep;
		pS += srcStep;
		pH0 += 8;
		pH1 += 8;
		pH2 += 8;
		pIC += 8;
		pIL += 8;
		pIR += 8;
		pIB += 8;
	}

	return ippStsNoErr;
}

