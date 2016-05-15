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

/*--------------------------------------------look here firstly---------------/ 
	here are functions like Intel Ipp declared in Ippvc.h. totally 31 useful
-----------------------------------------------thank you---------------------*/

#include "ippvc.h"
#include "kinoma_mpeg4_assistant.h"

#ifdef PRINT_REF_INFO
#include "dump.h"
#endif


/*----------------------------------------------------------------------------/
							Motion Compensation								  
----------------------------------------------------------------------------*/
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
		dump("ippiCopy8x8QP_MPEG4_8u_C1R_c		with acc(0--15): ",acc);
		dump("ippiCopy8x8QP_MPEG4_8u_C1R_c		with rounding(0,1): ",rounding);
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
#ifdef WIN32
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
#ifdef WIN32
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
#ifdef WIN32
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
ippiCopy8x4HP_8u_C1R_c(
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding);

IppStatus __STDCALL 
ippiCopy8x8HP_8u_C1R_c(							
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding);


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


/*----------------------------------------------------------------------------/
				Sprite and Global Motion Compensation
----------------------------------------------------------------------------*/
/*
WarpInit_MPEG4
Initializes IppiWarpSpec_MPEG4 structure for further
usage in GMC or Sprite reconstruction.
*/

IppStatus __STDCALL ippiWarpInit_MPEG4_c(
    IppiWarpSpec_MPEG4* pSpec,
    const int*          pDU,
    const int*          pDV,
    int                 numWarpingPoints,
    int                 spriteType,
    int                 warpingAccuracy,
    int                 roundingType,
    int                 quarterSample,
    int                 fcode,
    const IppiRect*     spriteRect,
    const IppiRect*     vopRect)
{
	int temp,temp1;
	int r_pwr,r;

	k_IppiWarpSpec_MPEG4 *pWarp = (k_IppiWarpSpec_MPEG4*)pSpec;

	if (!pSpec)
		return	ippStsNullPtrErr;
	if (spriteRect->width < 1 || spriteRect->height < 1 || vopRect->width < 1 || vopRect->height < 1)
		return ippStsSizeErr;
	if ((numWarpingPoints<0 || numWarpingPoints>4) || (warpingAccuracy<0 || warpingAccuracy >3) 
		|| (fcode < 1 || fcode > 7))
		return ippStsOutOfRangeErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiWarpInit_MPEG4		with numWarpingPoints(0,1,2,3,4): ",numWarpingPoints);
		dump("ippiWarpInit_MPEG4		with spriteType(0,1,2,): ",spriteType);
		dump("ippiWarpInit_MPEG4		with warpingAccuracy(0,1,2,3): ",warpingAccuracy);
		dump("ippiWarpInit_MPEG4		with roundingType(0,1): ",roundingType);
		dump("ippiWarpInit_MPEG4		with quarterSample(0,1): ",quarterSample);
		dump("ippiWarpInit_MPEG4		with fcode: ",fcode);
	}

#endif

	pWarp->numWarpingPoints = numWarpingPoints;
	pWarp->spriteType = spriteType;

	/* reference points in cur-vop */
	pWarp->vop_i0 = vopRect->x;
	pWarp->vop_j0 = vopRect->y;	
	pWarp->vop_i1 = pWarp->vop_i0 + vopRect->width;
	pWarp->vop_j1 = pWarp->vop_j0;
	pWarp->vop_i2 = pWarp->vop_i0;
	pWarp->vop_j2 = pWarp->vop_j0 + vopRect->height;
	pWarp->H = vopRect->height;
	pWarp->W = vopRect->width;
	pWarp->vop_i3 = pWarp->vop_i0 + pWarp->W;
	pWarp->vop_j3 = pWarp->vop_j0 + pWarp->H; 


	/* reference points in sprite-vop */
	pWarp->spr_i0 = ((pWarp->vop_i0<<1) + pDU[0]) << warpingAccuracy;		/*  multiply s/2 */
	pWarp->spr_j0 = ((pWarp->vop_j0<<1) + pDV[0]) << warpingAccuracy;
	pWarp->spr_i1 = ((pWarp->vop_i1<<1) + pDU[1] + pDU[0]) << warpingAccuracy;
	pWarp->spr_j1 = ((pWarp->vop_j1<<1) + pDV[1] + pDV[0]) << warpingAccuracy;
	pWarp->spr_i2 = ((pWarp->vop_i2<<1) + pDU[2] + pDU[0]) << warpingAccuracy;
	pWarp->spr_j2 = ((pWarp->vop_j2<<1) + pDV[2] + pDV[0]) << warpingAccuracy;
	pWarp->spr_i3 = ((pWarp->vop_i3<<1) + pDU[3] + pDU[2] + pDU[1] + pDU[0]) << warpingAccuracy;
	pWarp->spr_j3 = ((pWarp->vop_j3<<1) + pDV[3] + pDV[2] + pDV[1] + pDV[0]) << warpingAccuracy;


	/* 2*vop_i0 - 1,    2*vop_j0 - 1*/
	pWarp->ic = (pWarp->vop_i0 << 1) - 1;
	pWarp->jc = (pWarp->vop_j0 << 1) - 1;

	/* s , pwr_s, ps_round for the sample reconstruction */
	pWarp->s_pwr = warpingAccuracy+1;
	pWarp->s = 1 << pWarp->s_pwr;
	pWarp->ps_p = pWarp->s_pwr<<1;
	pWarp->pwr_s =  1 << pWarp->ps_p;//(pWarp->s_pwr<<1);
	pWarp->ps_round = (pWarp->pwr_s >> 1) - roundingType;
	pWarp->mv_half = 128 << pWarp->s_pwr; /* (256 * pWarp->s) / 2 */
	pWarp->mv_denom = 8 + pWarp->s_pwr;
	
	if (quarterSample) 
		pWarp->mv_scaler = 2;	/* using in left shift x4 */
	else
		pWarp->mv_scaler = 1;	/*  x2 */

	/*----------------- for clip after warping, btw, warping for GMV is not use clip */
	/*  luma topleft (i', j') = (sprite_left_coordinate, sprite_top_coordinate)  */
	pWarp->minYW = spriteRect->x;		
	pWarp->minYH = spriteRect->y;
	/*	luma bottomright
		(i', j') = (sprite_left_coordinate + sprite_width - 1,
					sprite_top_coordinate + sprite_height - 1)	*/
	pWarp->maxYW = (spriteRect->x+spriteRect->width-1) << (warpingAccuracy+1); /* (width-1) * s */
	pWarp->maxYH = (spriteRect->y+spriteRect->height-1) << (warpingAccuracy+1); /* (height-1) * s */

	/* chroma topleft (ic', jc') = (sprite_left_coordinate / 2, sprite_top_coordinate / 2)*/
	pWarp->minCW = spriteRect->x >> 1;
	pWarp->minCH = spriteRect->y >> 1;
	/* Bottom right chrominance sample
		(ic', jc') = (sprite_left_coordinate / 2 + sprite_width// 2 ? 1,
					  sprite_top_coordinate / 2 + sprite_height// 2 ? 1) */
	pWarp->maxCW = ((pWarp->minCW+((spriteRect->width+1)>>1)) - 1) << pWarp->s_pwr;
	pWarp->maxCH = ((pWarp->minCH+((spriteRect->height+1)>>1)) - 1) << pWarp->s_pwr;

	/*----------------------------------------clip warping end--------------------- */

	/* for GMV clip */
	pWarp->min = 1<<(fcode+4);		/* clip for gmv  */
	pWarp->max = pWarp->min - 1;
	pWarp->min = -pWarp->min;


	if (pWarp->numWarpingPoints == 1)
	{
		pWarp->wpt1_1 = (pWarp->spr_i0 >> 1) | (pWarp->spr_i0 & 1);
		pWarp->wpt1_2 = (pWarp->spr_j0 >> 1) | (pWarp->spr_j0 & 1);
	}
	/* calc the virtual sprite points */
	if (pWarp->numWarpingPoints > 1)
	{
		r = 16 >> (warpingAccuracy+1);
		pWarp->VW = 1; 
		pWarp->alpha = 0;
		while(pWarp->VW < pWarp->W) 
		{
			pWarp->VW <<= 1;
			pWarp->alpha++;
		}

		/* virtual reference points in sprite-vop */
		temp = ((pWarp->W - pWarp->VW) * (r*pWarp->spr_i0 - 16*pWarp->vop_i0) 
			+ pWarp->VW * (r*pWarp->spr_i1 - 16*pWarp->vop_i1));
		temp1 = (abs(temp) + pWarp->W/2) / pWarp->W;
		if (temp < 0)
			temp1 = -temp1;
		pWarp->spr_vi1 = 16 * (pWarp->vop_i0 + pWarp->VW) + temp1;
			
		temp = ((pWarp->W - pWarp->VW) * (r*pWarp->spr_j0 - 16*pWarp->vop_j0)
			+ pWarp->VW * (r*pWarp->spr_j1 - 16*pWarp->vop_j1));
		temp1 = (abs(temp) + pWarp->W/2) / pWarp->W;
		if (temp < 0)
			temp1 = -temp1;
		pWarp->spr_vj1 =  16 * pWarp->vop_j0 + temp1;

		pWarp->rou = 0;
		r_pwr = 1;
		while(r_pwr < r) 
		{
			r_pwr <<= 1;
			pWarp->rou++;
		}
	}

	if (pWarp->numWarpingPoints == 2)
	{
		/* for sprite_warping_points == 2 */
		/* (-r*spr_i0+spr_vi1),  (r*spr_j0-spr_vj1), (-r*spr_j0+spr_vj1), */
		pWarp->wpt2_1 = -r * pWarp->spr_i0 + pWarp->spr_vi1;  
		pWarp->wpt2_2 = r * pWarp->spr_j0 - pWarp->spr_vj1;
		pWarp->wpt2_3 = -r * pWarp->spr_j0 + pWarp->spr_vj1;	
		/* 2pow(alph+rou-1),  (alpha+rou) */
		pWarp->pwr_aArM1 = 1<<(pWarp->alpha + pWarp->rou - 1);	
		pWarp->aAr = pWarp->alpha + pWarp->rou;	

		/* for sprite_warping_points == 2 chroma*/
		/* (2*VW*r*spr_i0 -  16*VW + 2pow(alpha+rou+1))  */
		pWarp->wpt2_4 = 2 * pWarp->VW * r * pWarp->spr_i0 - 16 * pWarp->VW + (1<<(pWarp->alpha + pWarp->rou + 1));	
		/* (2*VW*r*spr_j0 -  16*VW + 2pow(alpha+rou+1))  */
		pWarp->wpt2_5 = 2 * pWarp->VW * r * pWarp->spr_j0 - 16 * pWarp->VW + (1<<(pWarp->alpha + pWarp->rou + 1));	
		/* (alpha+rou+2) */
		pWarp->aArA2 = pWarp->alpha + pWarp->rou + 2;					
	}
	else if (pWarp->numWarpingPoints == 3)
	{
		/* calc the virtual sprite points */
		pWarp->VH = 1; 
		pWarp->beta = 0;
		while(pWarp->VH < pWarp->H) 
		{
			pWarp->VH <<= 1;
			pWarp->beta++;
		}

		temp = ((pWarp->H - pWarp->VH) * (r*pWarp->spr_i0 - 16*pWarp->vop_i0)
			+ pWarp->VH * (r*pWarp->spr_i2 - 16*pWarp->vop_i2));;
		temp1 = (abs(temp) + pWarp->H/2) / pWarp->H; 
		if (temp < 0)
			temp1 = -temp1;
		pWarp->spr_vi2 = 16 * pWarp->vop_i0 + temp1;

		temp = ((pWarp->H - pWarp->VH) * (r*pWarp->spr_j0 - 16 * pWarp->vop_j0)
			+ pWarp->VH * (r*pWarp->spr_j2 - 16*pWarp->vop_j2));
		temp1 = (abs(temp) + pWarp->H/2) / pWarp->H; 
		if (temp < 0)
			temp1 = -temp1;
		pWarp->spr_vj2 = 16 * (pWarp->vop_j0 + pWarp->VH) + temp1 ;


		/* for sprite_warping_points == 3 */
		if (pWarp->VW >= pWarp->VH)
		{
			/* (-r*spr_i0+spr_vi1), (-r*spr_i0+spr_vi2)*2pow(alpha-beta) */
			pWarp->wpt3_1 = -r * pWarp->spr_i0 + pWarp->spr_vi1;		
			pWarp->wpt3_2 = (-r * pWarp->spr_i0 + pWarp->spr_vi2) * (1<<(pWarp->alpha-pWarp->beta));
			/* (-r*spr_j0+spr_vj1), (-r*spr_j0+spr_vj2)*2pow(alpha-beta) */
			pWarp->wpt3_3 = -r * pWarp->spr_j0 + pWarp->spr_vj1; 	
			pWarp->wpt3_4 = (-r * pWarp->spr_j0 + pWarp->spr_vj2) * (1<<(pWarp->alpha-pWarp->beta));

			/* for sprite_warping_points == 3 chroma*/
			/* (2*VW*r*spr_i0-16*VW+2pow(alpha+rou+1)) */
			pWarp->wpt3_5 = 2 * pWarp->VW * r * pWarp->spr_i0 - 16 * pWarp->VW + (1<<(pWarp->alpha + pWarp->rou + 1));	
			/* (2*VW*r*spr_j0-16*VW+2pow(alpha+rou+1)) */
			pWarp->wpt3_6 = 2 * pWarp->VW * r * pWarp->spr_j0 - 16 * pWarp->VW + (1<<(pWarp->alpha + pWarp->rou + 1));	

			/* 2pow(alph+rou-1),  (alpha+rou) */
			pWarp->pwr_aArM1 = 1<<(pWarp->alpha + pWarp->rou - 1);	
			pWarp->aAr = pWarp->alpha + pWarp->rou;	
			/* (alpha+rou+2) */
			pWarp->aArA2 = pWarp->alpha + pWarp->rou + 2;	
		}
		else
		{
			/* (-r*spr_i0+spr_vi1)*2pow(beta-alpha), (-r*spr_i0+spr_vi2) */
			pWarp->wpt3_1 = (-r * pWarp->spr_i0 + pWarp->spr_vi1) * (1<<(pWarp->beta - pWarp->alpha));		
			pWarp->wpt3_2 = (-r * pWarp->spr_i0 + pWarp->spr_vi2);
			/* (-r*spr_j0+spr_vj1)*2pow(beta-alpha), (-r*spr_j0+spr_vj2) */
			pWarp->wpt3_3 = (-r * pWarp->spr_j0 + pWarp->spr_vj1) * (1<<(pWarp->beta - pWarp->alpha)); 	
			pWarp->wpt3_4 = (-r * pWarp->spr_j0 + pWarp->spr_vj2);

			/* for sprite_warping_points == 3 chroma*/
			/* (2*VH*r*spr_i0-16*VH+2pow(beta+rou+1)) */
			pWarp->wpt3_5 = 2 * pWarp->VH * r * pWarp->spr_i0 - 16 * pWarp->VH + (1<<(pWarp->beta + pWarp->rou + 1));	
			/* (2*VH*r*spr_j0-16*VH+2pow(beta+rou+1)) */
			pWarp->wpt3_6 = 2 * pWarp->VH * r * pWarp->spr_j0 - 16 * pWarp->VH + (1<<(pWarp->beta + pWarp->rou + 1));	

			/* 2pow(beta+rou-1),  (beta+rou) */
			pWarp->pwr_aArM1 = 1<<(pWarp->beta + pWarp->rou - 1);	
			pWarp->aAr = pWarp->beta + pWarp->rou;	
			/* (beta+rou+2) */
			pWarp->aArA2 = pWarp->beta + pWarp->rou + 2;	
		}

	}
	else if (pWarp->numWarpingPoints == 4)
	{
		pWarp->g = ((pWarp->spr_i0-pWarp->spr_i1-pWarp->spr_i2+pWarp->spr_i3)*(pWarp->spr_j2-pWarp->spr_j3) 
			- (pWarp->spr_i2-pWarp->spr_i3)*(pWarp->spr_j0-pWarp->spr_j1-pWarp->spr_j2+pWarp->spr_j3)) *pWarp->H;  
		pWarp->h = ((pWarp->spr_i1-pWarp->spr_i3)*(pWarp->spr_j0-pWarp->spr_j1-pWarp->spr_j2+pWarp->spr_j3)
			- (pWarp->spr_i0-pWarp->spr_i1-pWarp->spr_i2+pWarp->spr_i3)*(pWarp->spr_j1-pWarp->spr_j3)) *pWarp->W;  
		pWarp->D = (pWarp->spr_i1-pWarp->spr_i3) *(pWarp->spr_j2-pWarp->spr_j3) - (pWarp->spr_i2-pWarp->spr_i3)*(pWarp->spr_j1-pWarp->spr_j3); 
		pWarp->a = pWarp->D*(pWarp->spr_i1 - pWarp->spr_i0)* pWarp->H + pWarp->g*pWarp->spr_i1;
		pWarp->b = pWarp->D*(pWarp->spr_i2 - pWarp->spr_i0)* pWarp->W + pWarp->h*pWarp->spr_i2; 
		pWarp->c = pWarp->D*pWarp->spr_i0*pWarp->W*pWarp->H;  
		pWarp->d = pWarp->D*(pWarp->spr_j1-pWarp->spr_j0)* pWarp->H + pWarp->g*pWarp->spr_j1;  
		pWarp->e = pWarp->D*(pWarp->spr_j2-pWarp->spr_j0)* pWarp->W + pWarp->h*pWarp->spr_j2;  
		pWarp->f = pWarp->D*pWarp->spr_j0*pWarp->W*pWarp->H; 
	}

	return ippStsNoErr;
}


/*
WarpGetSize_MPEG4
Returns size of IppiWarpSpec_MPEG4 structure.
*/
IppStatus __STDCALL 
ippiWarpGetSize_MPEG4_c(
    int*  pSpecSize)
{
	if (!pSpecSize)
		return	ippStsNullPtrErr;
	*pSpecSize = sizeof(k_IppiWarpSpec_MPEG4);

#ifdef PRINT_REF_INFO
	dump("ippiWarpGetSize_MPEG4",-1);
#endif

	return ippStsNoErr;

}


/*
WarpLuma_MPEG4
Warps arbitrary rectangular luminance region.
*/
IppStatus __STDCALL 
ippiWarpLuma_MPEG4_8u_C1R_c(
    const Ipp8u*              pSrcY,
    int                       srcStepY,
    Ipp8u*                    pDstY,
    int                       dstStepY,
    const IppiRect*           dstRect,
    const IppiWarpSpec_MPEG4* pSpec)
{
	int i,j,c, ri, rj,ti,tj;
	int f[256],g[256];
	Ipp8u *pD;
	int pos;
	float temp;
	k_IppiWarpSpec_MPEG4 *pWarp = (k_IppiWarpSpec_MPEG4*)pSpec;

#ifdef PRINT_REF_INFO
	{
		dump("ippiWarpLuma_MPEG4_8u_C1R		with numWarpingPoints(0,1,2,3,4): ",pWarp->numWarpingPoints);
		dump("ippiWarpLuma_MPEG4_8u_C1R		with spriteType(0,1,2,): ",pWarp->spriteType);
	}
#endif

	c = 0;
	if (pWarp->numWarpingPoints == 0)
	{
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				/*f[c] = pWarp->s * i;
				g[c] = pWarp->s * j;*/
				f[c] = i << pWarp->s_pwr;
				g[c] = j << pWarp->s_pwr;
				f[c] = CLIP_R(f[c],pWarp->maxYW,pWarp->minYW);
				g[c] = CLIP_R(g[c],pWarp->maxYH,pWarp->minYH);
				c++;
			}
		}
	}
	else if (pWarp->numWarpingPoints == 1)
	{
		int jw,iw;
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			jw = pWarp->spr_j0 + ((j-pWarp->vop_j0)<< pWarp->s_pwr);
			iw = dstRect->x-pWarp->vop_i0;
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				/*f[c] = pWarp->spr_i0 + pWarp->s * (i-pWarp->vop_i0);
				g[c] = pWarp->spr_j0 + pWarp->s * (j-pWarp->vop_j0);*/
				f[c] = pWarp->spr_i0 + (iw<< pWarp->s_pwr);
				g[c] = jw;
				f[c] = CLIP_R(f[c],pWarp->maxYW,pWarp->minYW);
				g[c] = CLIP_R(g[c],pWarp->maxYH,pWarp->minYH);
				c++;
				iw++;
			}
		}
	}
	else if (pWarp->numWarpingPoints == 2)
	{
		int jw2,jw1,iw;
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			jw2 = pWarp->wpt2_2 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			jw1 = pWarp->wpt2_1 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			iw = dstRect->x-pWarp->vop_i0;
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				f[c] = pWarp->spr_i0 + ((pWarp->wpt2_1 * iw + jw2) >> pWarp->aAr);
				g[c] = pWarp->spr_j0 + ((pWarp->wpt2_3 * iw + jw1) >> pWarp->aAr);
				f[c] = CLIP_R(f[c],pWarp->maxYW,pWarp->minYW);
				g[c] = CLIP_R(g[c],pWarp->maxYH,pWarp->minYH);
				c++;
				iw++;
			}
		}	
	}
	else if (pWarp->numWarpingPoints == 3)
	{
		int jw2,jw4,iw;
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			jw2 = pWarp->wpt3_2 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			jw4 = pWarp->wpt3_4 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			iw = dstRect->x - pWarp->vop_i0;
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				f[c] = pWarp->spr_i0 + ((pWarp->wpt3_1 * iw + jw2) >> pWarp->aAr);
				g[c] = pWarp->spr_j0 + ((pWarp->wpt3_3 * iw + jw4) >> pWarp->aAr);
				f[c] = CLIP_R(f[c],pWarp->maxYW,pWarp->minYW);		
				g[c] = CLIP_R(g[c],pWarp->maxYH,pWarp->minYH);
				c++;
				iw++;
			}
		}	
	}
	else		/* (pWarp->numWarpingPoints == 4) */
	{
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				temp = (float)(pWarp->a * (i - pWarp->vop_i0) + pWarp->b * (j - pWarp->vop_j0) + pWarp->c) / 
					(pWarp->g * (i - pWarp->vop_i0) + pWarp->h * (j - pWarp->vop_j0) + pWarp->D * pWarp->W * pWarp->H);
				f[c] = (int)(temp < 0 ? (temp): (temp+ 0.5));
				temp = (float)(pWarp->d * (i - pWarp->vop_i0) + pWarp->e * (j - pWarp->vop_j0) + pWarp->f) /
					(pWarp->g * (i - pWarp->vop_i0) + pWarp->h * (j - pWarp->vop_j0) + pWarp->D * pWarp->W * pWarp->H);
				g[c] = (int)(temp < 0 ? (temp): (temp+ 0.5));
				f[c] = CLIP_R(f[c],pWarp->maxYW,pWarp->minYW);
				g[c] = CLIP_R(g[c],pWarp->maxYH,pWarp->minYH);
				c++;
			}
		}	
	}

	c = 0;
	pD = pDstY;
	if (pWarp->spriteType == IPPVC_SPRITE_STATIC)
	{
		for ( j = 0; j < dstRect->height; j++)
		{
			for (i = 0; i < dstRect->width; i++)
			{
				ti = f[c]>>pWarp->s_pwr;		
				tj = g[c]>>pWarp->s_pwr;
				ri = f[c] - (ti<<pWarp->s_pwr);	
				rj = g[c] - (tj<<pWarp->s_pwr);	
				pos = ti+tj*srcStepY;
				temp = ((pWarp->s - rj) * ((pWarp->s - ri) * pSrcY[pos] + ri * pSrcY[pos+1]) 
					+ rj * ((pWarp->s - ri) * pSrcY[pos+srcStepY] + ri * pSrcY[pos+1+srcStepY])) / (float)pWarp->pwr_s;
				pD[i] = (Ipp8u)(temp < 0 ? (temp-0.5) : (temp+0.5));
				c++;
			}
			pD += dstStepY;
		}
	}
	else
	{
		for ( j = 0; j < dstRect->height; j++)
		{
			for (i = 0; i < dstRect->width; i++)
			{
				ti = f[c]>>pWarp->s_pwr;		/* f[c] //// s */
				tj = g[c]>>pWarp->s_pwr;
				ri = f[c] - (ti<<pWarp->s_pwr);	
				rj = g[c] - (tj<<pWarp->s_pwr);	
				pos = ti+tj*srcStepY;
				pD[i] = ((pWarp->s - rj) * ((pSrcY[pos]<<pWarp->s_pwr) + ri * (pSrcY[pos+1]-pSrcY[pos])) 
					+ rj * ((pSrcY[pos+srcStepY]<<pWarp->s_pwr) + ri * (pSrcY[pos+1+srcStepY]-pSrcY[pos+srcStepY])) + pWarp->ps_round)>>pWarp->ps_p;/* /pWarp->pwr_s;*/
				
				/*pD[i] = ((pWarp->s - rj) * ((pWarp->s - ri) * pSrcY[pos] + ri * pSrcY[pos+1]) 
					+ rj * ((pWarp->s - ri) * pSrcY[pos+srcStepY] + ri * pSrcY[pos+1+srcStepY]) + pWarp->ps_round)>>pWarp->ps_p;/* /pWarp->pwr_s;*/
				c++;
			}
			pD += dstStepY;
		}
	}

	return ippStsNoErr;
}


/*
WarpChroma_MPEG4
Warps arbitrary rectangular chrominance region.
*/
IppStatus __STDCALL 
ippiWarpChroma_MPEG4_8u_P2R_c(
    const Ipp8u*              pSrcCb,
    int                       srcStepCb,
    const Ipp8u*              pSrcCr,
    int                       srcStepCr,
    Ipp8u*                    pDstCb,
    int                       dstStepCb,
    Ipp8u*                    pDstCr,
    int                       dstStepCr,
    const IppiRect*           dstRect,
    const IppiWarpSpec_MPEG4* pSpec)
{
	int i,j,c,posb,posr, ri, rj,ti,tj;
	int f[64],g[64];
	Ipp8u *pDu, *pDv;
	float temp;
	k_IppiWarpSpec_MPEG4 *pWarp = (k_IppiWarpSpec_MPEG4*)pSpec;

#ifdef PRINT_REF_INFO
	{
		dump("ippiWarpChroma_MPEG4_8u_P2R			with numWarpingPoints(0,1,2,3,4): ",pWarp->numWarpingPoints);
		dump("ippiWarpChroma_MPEG4_8u_P2R			with spriteType(0,1,2,): ",pWarp->spriteType);
	}
#endif

	c = 0;
	if (pWarp->numWarpingPoints == 0)
	{
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				//f[c] = pWarp->s * i;
				//g[c] = pWarp->s * j;
				f[c] = i << pWarp->s_pwr;
				g[c] = j << pWarp->s_pwr;
				f[c] = CLIP_R(f[c],pWarp->maxCW,pWarp->minCW);
				g[c] = CLIP_R(g[c],pWarp->maxCH,pWarp->minCH);
				c++;
			}
		}	
	}
	else if (pWarp->numWarpingPoints == 1)
	{
		if ( pWarp->spriteType == IPPVC_SPRITE_STATIC)
		{
			for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
			{
				for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
				{
					temp = pWarp->spr_i0 / 2.0f;
					temp = temp < 0? (temp) : (temp+0.5f);
					//f[c] = (int)(temp + pWarp->s * (i - pWarp->vop_i0/2));
					f[c] = (int)(temp + ((i - pWarp->vop_i0/2)<<pWarp->s_pwr));
					temp = pWarp->spr_j0 / 2.0f;
					temp = temp < 0? (temp) : (temp+0.5f);
					//g[c] = (int)(temp + pWarp->s * (j - pWarp->vop_j0/2));
					g[c] = (int)(temp + ((j - pWarp->vop_j0/2)<<pWarp->s_pwr));
					f[c] = CLIP_R(f[c],pWarp->maxCW,pWarp->minCW);
					g[c] = CLIP_R(g[c],pWarp->maxCH,pWarp->minCH);
					c++;
				}
			}
		}
		else
		{
			int jw, iw;
			for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
			{
				jw = pWarp->wpt1_2 + ((j - pWarp->vop_j0/2)<<pWarp->s_pwr);
				iw = (dstRect->x - pWarp->vop_i0/2);
				for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
				{
					/*f[c] = ((pWarp->spr_i0 >> 1) | (pWarp->spr_i0 & 1)) + pWarp->s * (i - pWarp->vop_i0/2);
					g[c] = ((pWarp->spr_j0 >> 1) | (pWarp->spr_j0 & 1)) + pWarp->s * (j - pWarp->vop_j0/2);*/
					f[c] = pWarp->wpt1_1 + (iw<<pWarp->s_pwr);
					g[c] = jw;
					f[c] = CLIP_R(f[c],pWarp->maxCW,pWarp->minCW);
					g[c] = CLIP_R(g[c],pWarp->maxCH,pWarp->minCH);
					c++;
					iw++;
				}
			}
		}	
	}
	else if (pWarp->numWarpingPoints == 2)
	{
		int jw2,jw1, iw;
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			jw2 = pWarp->wpt2_2 * ((j << 2) - pWarp->jc) + pWarp->wpt2_4;
			jw1 = pWarp->wpt2_1 * ((j << 2) - pWarp->jc) + pWarp->wpt2_5;
			iw = (dstRect->x<<2) - pWarp->ic;
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				/*f[c] = (pWarp->wpt2_1 * (4*i - pWarp->ic) + pWarp->wpt2_2 * (4*j - pWarp->jc) + pWarp->wpt2_4) >> pWarp->aArA2;
				g[c] = (pWarp->wpt2_3 * (4*i - pWarp->ic) + pWarp->wpt2_1 * (4*j - pWarp->jc) + pWarp->wpt2_5) >> pWarp->aArA2;*/
				f[c] = (pWarp->wpt2_1 * iw + jw2) >> pWarp->aArA2;
				g[c] = (pWarp->wpt2_3 * iw + jw1) >> pWarp->aArA2;
				f[c] = CLIP_R(f[c],pWarp->maxCW,pWarp->minCW);
				g[c] = CLIP_R(g[c],pWarp->maxCH,pWarp->minCH);
				c++;
				iw += 4;
			}
		}	
	}
	else if (pWarp->numWarpingPoints == 3)
	{
		int jw2,jw4, iw;
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			jw2 = pWarp->wpt3_2 * ((j << 2) - pWarp->jc) + pWarp->wpt3_5;
			jw4 = pWarp->wpt3_4 * ((j << 2) - pWarp->jc) + pWarp->wpt3_6;
			iw = (dstRect->x<<2) - pWarp->ic;
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				/*f[c] = ((pWarp->wpt3_1 * (4*i - pWarp->ic) + pWarp->wpt3_2 * (4*j - pWarp->jc) + pWarp->wpt3_5) >> pWarp->aArA2);
				g[c] = ((pWarp->wpt3_3 * (4*i - pWarp->ic) + pWarp->wpt3_4 * (4*j - pWarp->jc) + pWarp->wpt3_6) >> pWarp->aArA2);*/
				f[c] = (pWarp->wpt3_1 * iw + jw2) >> pWarp->aArA2;
				g[c] = (pWarp->wpt3_3 * iw + jw4) >> pWarp->aArA2;
				f[c] = CLIP_R(f[c],pWarp->maxCW,pWarp->minCW);
				g[c] = CLIP_R(g[c],pWarp->maxCH,pWarp->minCH);
				c++;
				iw += 4;
			}
		}	
	}
	else		/*(pWarp->numWarpingPoints == 4) */
	{
		for (j = dstRect->y; j < dstRect->y+dstRect->height; j++)
		{
			for(i = dstRect->x; i < dstRect->x+dstRect->width; i++)
			{
				temp = (float)(2 * pWarp->a * (4*i - pWarp->ic) + 2 * pWarp->b * (4*j - pWarp->jc) + 4* pWarp->c -
					(pWarp->g * (4*i - pWarp->ic) + pWarp->h * (4*j - pWarp->jc) + 2 * pWarp->D * pWarp->W * pWarp->H) * pWarp->s)
					/ (4 * pWarp->g * (4*i - pWarp->ic) + 4 * pWarp->h * (4*j - pWarp->jc) + 8 * pWarp->D * pWarp->W * pWarp->H);
				f[c] = (int)(temp < 0 ? (temp): (temp+ 0.5));
				temp = (float)(2 * pWarp->d * (4*i - pWarp->ic) + 2 * pWarp->e * (4*j - pWarp->jc) + 4* pWarp->f -
					(pWarp->g * (4*i - pWarp->ic) + pWarp->h * (4*j - pWarp->jc) + 2 * pWarp->D * pWarp->W * pWarp->H) * pWarp->s)
					/ (4 * pWarp->g * (4*i - pWarp->ic) + 4 * pWarp->h * (4*j - pWarp->jc) + 8 * pWarp->D * pWarp->W * pWarp->H);
				g[c] = (int)(temp < 0 ? (temp): (temp+ 0.5));
				f[c] = CLIP_R(f[c],pWarp->maxCW,pWarp->minCW);
				g[c] = CLIP_R(g[c],pWarp->maxCH,pWarp->minCH);
				c++;
			}
		}	
	}

	c = 0;
	pDu = pDstCb;
	pDv = pDstCr;
	if (pWarp->spriteType == IPPVC_SPRITE_STATIC)
	{
		for ( j = 0; j < dstRect->height; j++)
		{
			for (i = 0; i < dstRect->width; i++)
			{
				ti = f[c]>>pWarp->s_pwr;	/* f[c]////s */
				tj = g[c]>>pWarp->s_pwr;
				ri = f[c] - (ti<<pWarp->s_pwr);	
				rj = g[c] - (tj<<pWarp->s_pwr);	
				posb = ti+tj*srcStepCb;
				posr = ti+tj*srcStepCr;
				temp = ((pWarp->s - rj) * ((pWarp->s - ri) * pSrcCb[posb] + ri * pSrcCb[posb+1]) 
					+ rj * ((pWarp->s - ri) * pSrcCb[posb+srcStepCb] + ri * pSrcCb[posb+1+srcStepCb])) / (float)pWarp->pwr_s;//(pWarp->s*pWarp->s);
				pDu[i] = (Ipp8u)(temp < 0 ? (temp-0.5) : (temp+0.5));

				temp = ((pWarp->s - rj) * ((pWarp->s - ri) * pSrcCr[posr] + ri * pSrcCr[posr+1]) 
					+ rj * ((pWarp->s - ri) * pSrcCr[posr+srcStepCr] + ri * pSrcCr[posr+1+srcStepCr])) / (float)pWarp->pwr_s;//(pWarp->s*pWarp->s);
				pDv[i] = (Ipp8u)(temp < 0 ? (temp-0.5) : (temp+0.5));
				c++;
			}
			pDu += dstStepCb;
			pDv += dstStepCr;
		}
	}
	else
	{
		for ( j = 0; j < dstRect->height; j++)
		{
			for (i = 0; i < dstRect->width; i++)
			{
				ti = f[c]>>pWarp->s_pwr;	/* f[c]////s */
				tj = g[c]>>pWarp->s_pwr;
				ri = f[c] - (ti<<pWarp->s_pwr);	
				rj = g[c] - (tj<<pWarp->s_pwr);	
				posb = ti+tj*srcStepCb;
				posr = ti+tj*srcStepCr;
				pDu[i] = ((pWarp->s - rj) * ((pSrcCb[posb]<<pWarp->s_pwr) + ri * (pSrcCb[posb+1]-pSrcCb[posb])) 
					+ rj * ((pSrcCb[posb+srcStepCb]<<pWarp->s_pwr) + ri * (pSrcCb[posb+1+srcStepCb]-pSrcCb[posb+srcStepCb])) + pWarp->ps_round) >> pWarp->ps_p; /* /pWarp->pwr_s;*/

				/*pDu[i] = ((pWarp->s - rj) * ((pWarp->s - ri) * pSrcCb[posb] + ri * pSrcCb[posb+1]) 
					+ rj * ((pWarp->s - ri) * pSrcCb[posb+srcStepCb] + ri * pSrcCb[posb+1+srcStepCb]) + pWarp->ps_round) >> pWarp->ps_p; /* /pWarp->pwr_s;*/

				pDv[i] = ((pWarp->s - rj) * ((pSrcCr[posr]<<pWarp->s_pwr) + ri * (pSrcCr[posr+1]-pSrcCr[posr])) 
					+ rj * ((pSrcCr[posr+srcStepCr]<<pWarp->s_pwr) + ri * (pSrcCr[posr+1+srcStepCr]-pSrcCr[posr+srcStepCr])) + pWarp->ps_round) >> pWarp->ps_p; 

				/*pDv[i] = ((pWarp->s - rj) * ((pWarp->s - ri) * pSrcCr[posr] + ri * pSrcCr[posr+1]) 
					+ rj * ((pWarp->s - ri) * pSrcCr[posr+srcStepCr] + ri * pSrcCr[posr+1+srcStepCr]) + pWarp->ps_round) >> pWarp->ps_p; */

				c++;
			}
			pDu += dstStepCb;
			pDv += dstStepCr;
		}

	}

	return ippStsNoErr;
}



/*
CalcGlobalMV_MPEG4
Calculates Global Motion Vector for one macroblock.
*/

IppStatus __STDCALL 
ippiCalcGlobalMV_MPEG4_c(
    int                        xOffset,
    int                        yOffset,
    IppMotionVector*           pGMV,
    const IppiWarpSpec_MPEG4*  pSpec)
{
	int i,j,c;
	int f[256],g[256];
	int sumx, sumy, tmp;

	k_IppiWarpSpec_MPEG4 *pWarp = (k_IppiWarpSpec_MPEG4*)pSpec;

	if (!pWarp)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCalcGlobalMV_MPEG4		with numWarpingPoints(0,1,2,3): ",pWarp->numWarpingPoints);
	}
#endif

	/* calc the warping points in sprite vop---no clip */
	c = 0;
	if (pWarp->numWarpingPoints == 0)
	{
		for (j = yOffset; j < yOffset+16; j++)
		{
			for(i = xOffset; i < xOffset+16; i++)
			{
				/*f[c] = pWarp->s * i;
				g[c] = pWarp->s * j;*/
				f[c] = i << pWarp->s_pwr;
				g[c] = j << pWarp->s_pwr;
				c++;
			}
		}	
	}
	else if (pWarp->numWarpingPoints == 1)
	{
		int jw, iw;
		for (j = yOffset; j < yOffset+16; j++)
		{
			jw =  pWarp->spr_j0 + ((j-pWarp->vop_j0) << pWarp->s_pwr);
			iw = xOffset-pWarp->vop_i0;
			for(i = xOffset; i < xOffset+16; i++)
			{
				/*f[c] = pWarp->spr_i0 + pWarp->s * (i-pWarp->vop_i0);
				g[c] = pWarp->spr_j0 + pWarp->s * (j-pWarp->vop_j0);*/
				f[c] = pWarp->spr_i0 + (iw<< pWarp->s_pwr);
				g[c] = jw;
				c++;
				iw++;
			}
		}
	}
	else if (pWarp->numWarpingPoints == 2)
	{
		int jw1,jw2, iw;
		for (j = yOffset; j < yOffset+16; j++)
		{
			jw2 = pWarp->wpt2_2 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			jw1 = pWarp->wpt2_1 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			iw = xOffset-pWarp->vop_i0;
			for(i = xOffset; i < xOffset+16; i++)
			{
				f[c] = pWarp->spr_i0 + ((pWarp->wpt2_1 * iw + jw2) >> pWarp->aAr);
				g[c] = pWarp->spr_j0 + ((pWarp->wpt2_3 * iw + jw1) >> pWarp->aAr);
				c++;
				iw++;
			}
		}	
	}
	else if (pWarp->numWarpingPoints == 3)
	{
		int jw2, jw4, iw;
		for (j = yOffset; j < yOffset+16; j++)
		{
			jw2 = pWarp->wpt3_2 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			jw4 = pWarp->wpt3_4 * (j-pWarp->vop_j0) + pWarp->pwr_aArM1;
			iw = xOffset-pWarp->vop_i0;
			for(i = xOffset; i < xOffset+16; i++)
			{
				f[c] = pWarp->spr_i0 + ((pWarp->wpt3_1 * iw + jw2) >> pWarp->aAr);
				g[c] = pWarp->spr_j0 + ((pWarp->wpt3_3 * iw + jw4) >> pWarp->aAr);
				c++;
				iw++;
			}
		}
	}
	/* from VM encode command parsing code, we can see the numWarpingpoints == 4 is not concomitant with GMV */
	/*
	else if (pWarp->numWarpingPoints == 4)
	{
		for (j = yOffset; j < yOffset+16; j++)
		{
			for(i = xOffset; i < xOffset+16; i++)
			{
				temp = (double)(pWarp->a * (i - pWarp->vop_i0) + pWarp->b * (j - pWarp->vop_j0) + pWarp->c) / 
					(pWarp->g * (i - pWarp->vop_i0) + pWarp->h * (j - pWarp->vop_j0) + pWarp->D * pWarp->W * pWarp->H);
				f[c] = (int)(temp < 0 ? (temp): (temp+ 0.5));
				temp = (double)(pWarp->d * (i - pWarp->vop_i0) + pWarp->e * (j - pWarp->vop_j0) + pWarp->f) /
					(pWarp->g * (i - pWarp->vop_i0) + pWarp->h * (j - pWarp->vop_j0) + pWarp->D * pWarp->W * pWarp->H);
				g[c] = (int)(temp < 0 ? (temp): (temp+ 0.5));
				c++;
			}
		}	
	}
	*/

	/* calc the mv of every points and the sum*/
	sumx = sumy = 0;
	c = 0;

	for (j = yOffset; j < yOffset+16; j++)
	{
		tmp = j<<pWarp->s_pwr;
		for (i = xOffset; i < xOffset+16; i++)
		{
			sumx += f[c] - (i<<pWarp->s_pwr);
			sumy += g[c] - tmp;
			c++;
		}
	}

	/*
		GMV = ( sum * quarter_scaler ) // ( 256 * s)
	*/
	pGMV->dx = ((abs(sumx)<<pWarp->mv_scaler) + pWarp->mv_half) >> pWarp->mv_denom;	 /* 8 + pWarp->s_pwr;*/
	if (sumx < 0)
		pGMV->dx = -pGMV->dx;
	pGMV->dy = ((abs(sumy)<<pWarp->mv_scaler) + pWarp->mv_half) >> pWarp->mv_denom;		
	if (sumy < 0)
		pGMV->dy = -pGMV->dy;

	pGMV->dx = CLIP_R(pGMV->dx,pWarp->max,pWarp->min);
	pGMV->dy = CLIP_R(pGMV->dy,pWarp->max,pWarp->min);

	return ippStsNoErr;
}


/*
ChangeSpriteBrightness_MPEG4
Change brightness after sprite warping.
*/

IppStatus __STDCALL 
ippiChangeSpriteBrightness_MPEG4_8u_C1IR_c(
    Ipp8u*  pSrcDst,
    int     srcDstStep,
    int     width,
    int     height,
    int     brightnessChangeFactor)
{
	/*Y =(Y * (brightness_change_factor + 100)) // 100, clipped to the range of [0, 2pow(bits_per_pixel)-1].*/
	int i, j;
	float temp;
	Ipp8u * pS;

	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (brightnessChangeFactor < -112 || brightnessChangeFactor > 1648)
		return ippStsOutOfRangeErr;

#ifdef PRINT_REF_INFO
	dump("ippiChangeSpriteBrightness_MPEG4_8u_C1IR",-1);
#endif

	pS = pSrcDst;
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			temp = (pS[i] * (brightnessChangeFactor + 100)) / 100.0f;
			pS[i] = (int)(temp < 0? (temp-0.5): (temp+0.5));
			pS[i] = CLIP(pS[i]);
		}
		pS += srcDstStep;
	}

	return ippStsNoErr;
}



/*----------------------------------------------------------------------------/
							Inverse Quantization
----------------------------------------------------------------------------*/

/*
QuantInvIntraInit_MPEG4,
QuantInvInterInit_MPEG4
Initialize specification structures.
*/

/**************************************************************
*   NEED to confirm the structures, it's a problem for latter *
*   the current code is a hypothesis						  *
**************************************************************/
IppStatus __STDCALL 
ippiQuantInvIntraInit_MPEG4_c(								/* done */
    const Ipp8u*                 pQuantMatrix,
    IppiQuantInvIntraSpec_MPEG4* pSpec,
    int                          bitsPerPixel)
{
	k_IppiQuantInvIntraSpec_MPEG4 * pQuant = (k_IppiQuantInvIntraSpec_MPEG4*)pSpec;
	/* pQuantMatrix can be NULL*/
	if (!pSpec)
		return ippStsNullPtrErr;

	pQuant->quant_max = (1<<(bitsPerPixel-3)) - 1;
	pQuant->min = (1 << (bitsPerPixel+3));
	pQuant->max = pQuant->min - 1;
	pQuant->min = -pQuant->min;

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvIntraInit_MPEG4",-1);
#endif

	if (NULL != pQuantMatrix)
	{
		pQuant->bUseMat = 1;
		memcpy(pQuant->matrix, pQuantMatrix,64);
#ifdef _DEBUG
		printf("the Intra quant matrix is:\n");
		//***print(pQuantMatrix,8,8,8);
#endif
	}
	else 
		pQuant->bUseMat = 0;

	return ippStsNoErr;

}


IppStatus __STDCALL 
ippiQuantInvInterInit_MPEG4_c(								
    const Ipp8u*                 pQuantMatrix,
    IppiQuantInvInterSpec_MPEG4* pSpec,
    int                          bitsPerPixel)
{
	k_IppiQuantInvInterSpec_MPEG4 * pQuant = (k_IppiQuantInvInterSpec_MPEG4*)pSpec;
	/* pQuantMatrix can be NULL*/
	if (!pSpec)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvInterInit_MPEG4",-1);
#endif
	pQuant->quant_max = (1<<(bitsPerPixel-3)) - 1;
	pQuant->min = (1 << (bitsPerPixel+3));
	pQuant->max =  pQuant->min -1;
	pQuant->min = -pQuant->min;

	if (NULL != pQuantMatrix)
	{
		pQuant->bUseMat = 1;
		memcpy(pQuant->matrix,pQuantMatrix,64);

#ifdef _DEBUG
		printf("the Inter quant matrix is:\n");
		//***print(pQuantMatrix,8,8,8);
#endif
	}
	else 
		pQuant->bUseMat = 0;


	return ippStsNoErr;
}


/*
QuantInvIntraGetSize_MPEG4,
QuantInvInterGetSize_MPEG4
Return size of specification structures.
*/
IppStatus __STDCALL 
ippiQuantInvIntraGetSize_MPEG4_c(								
    int* pSpecSize)
{
	if (!pSpecSize)
		return ippStsNullPtrErr;
	*pSpecSize = sizeof(k_IppiQuantInvIntraSpec_MPEG4);

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvIntraGetSize_MPEG4",-1);
#endif;

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiQuantInvInterGetSize_MPEG4_c(								
    int* pSpecSize)
{
	if (!pSpecSize)
		return ippStsNullPtrErr;
	*pSpecSize = sizeof(k_IppiQuantInvInterSpec_MPEG4);

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvInterGetSize_MPEG4",-1);
#endif
	return ippStsNoErr;
}


/*
QuantInvIntra_MPEG4,
QuantInvInter_MPEG4
Perform inverse quantization on intra/inter coded
block.
*/
IppStatus __STDCALL 
ippiQuantInvIntra_MPEG4_16s_C1I_c(								
    Ipp16s*                            pCoeffs,
    int                                indxLastNonZero,
    const IppiQuantInvIntraSpec_MPEG4* pSpec,
    int                                QP,
    int                                blockType)
{
	int j,sum, dc_scaler;
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);
	Ipp16s				tmpCoeff;

	k_IppiQuantInvIntraSpec_MPEG4 *pQuant = (k_IppiQuantInvIntraSpec_MPEG4*) pSpec;

	if (!pCoeffs || !pSpec)
		return ippStsNullPtrErr;
	
	if (QP < 1 || QP > pQuant->quant_max)
		return ippStsQPErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiQuantInvIntra_MPEG4_16s_C1I		 with blockType(0,1): ",blockType);
		dump("ippiQuantInvIntra_MPEG4_16s_C1I		 with quant_type(0,1): ",pQuant->bUseMat);
	}
#endif

	/* Intra DC */
	if (IPPVC_BLOCK_LUMA == blockType)
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP < 9) dc_scaler = QP << 1; /* 2* QP */
		else if (QP < 25) dc_scaler = QP + 8;
		else dc_scaler = (QP << 1) - 16;	/* 2 * QP - 16; */
	}
	else
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP < 25) dc_scaler = (QP + 13) >> 1;	/*((QP + 13) / 2;)*/
		else dc_scaler = QP - 6;
	} 
	pCoeffs[0] *= dc_scaler;
	/* Intra DC Inverse quantization */

	if (pQuant->bUseMat)	 /*1 == quant_type*/
	{
		sum=0;
		for (j = 1; j < 64; j++)
		{
			if (0 != pCoeffs[j])
			{
				pCoeffs[j] = ((pCoeffs[j]) * pQuant->matrix[j] * QP)/8; 
				//pCoeffs[j] = ((2* pCoeffs[j]) * pQuant->matrix[j] * QP)/16; 

				pCoeffs[j] = CLIP_R(pCoeffs[j],pQuant->max,pQuant->min);
				sum += pCoeffs[j];
			}/* if pCoeff is 0, we need do Nothing.*/
		}

		/*mismatch control*/
		sum += pCoeffs[0];
		if ((sum & 0x1) == 0) 
			pCoeffs[63] += ((pCoeffs[63] & 0x1)? -1:1); 
	}
	else	/*0 == quant_type */
	{
		for (j = 1; j < 64; j++)
		{
			if (0 != pCoeffs[j])
			{
				if (pCoeffs[j] < 0) 
				{
					tmpCoeff = pCoeffs[j]*QP_1 - QP_2;
					pCoeffs[j] = (tmpCoeff < pQuant->min ? pQuant->min : tmpCoeff );
				} else 
				{
					tmpCoeff = pCoeffs[j]*QP_1 + QP_2;
					pCoeffs[j] = (tmpCoeff > pQuant->max ? pQuant->max : tmpCoeff );
				}		
			} /* if pCoeff is 0, we need do Nothing.*/
		}
	}

	return ippStsNoErr;
}



/*----------------------------------------------------------------------------/
							VLC Decoding
----------------------------------------------------------------------------*/
/*
DecodeDCIntra_MPEG4
Decodes one DC coefficient for intra coded block.
*/
IppStatus __STDCALL 
ippiDecodeDCIntra_MPEG4_1u16s_c(Ipp8u **ppBitStream, int *pBitOffset,	
							  Ipp16s *pDC, int blockType)
{
	Ipp32u dc_size;
	Ipp32u code;
	int first_bit;

	if (!ppBitStream || !pBitOffset)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

#ifdef PRINT_REF_INFO	
	dump("ippiDecodeDCIntra_MPEG4_1u16s", -1);
#endif

	/* read DC size 2 - 8 bits */
	dc_size = VlcDecIntraDCPredSize(ppBitStream, pBitOffset, blockType);

	if (dc_size == 0)
	{
		*pDC = 0;
	}
	else
	{
		/* read delta DC 0 - 8 bits */
		code = k_mp4_GetBits(ppBitStream, pBitOffset, dc_size);

		first_bit = code >> (dc_size-1);
		if (first_bit == 0 )
		{ /* negative delta INTRA DC */
			*pDC  = -1*(code ^ ((1 << dc_size) - 1));
		}
		else
		{ /* positive delta INTRA DC */
			*pDC = code;
		}
		if (dc_size > 8)
		{
#ifdef _DEBUG
			printf("dc_size > 8 in ippiDecodeDCIntra_MPEG4_1u16s\n\n");
#endif
			(void) k_mp4_FlushBits(ppBitStream, pBitOffset, 1);
		}
	}

	return ippStsNoErr;
}



/*
DecodeCoeffsIntra_MPEG4
Decodes DCT coefficients for intra coded block.
*/
IppStatus __STDCALL 
ippiDecodeCoeffsIntra_MPEG4_1u16s_c(						
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    Ipp16s*  pCoeffs,
    int*     pIndxLastNonZero,
    int      rvlcFlag,
    int      noDCFlag,
    int      scan)
{
	Ipp32s              i;
	Tcoef               run_level;
	const Ipp32s        *pZigzag = zigzag[scan+1];

	if (!ppBitStream || !pBitOffset || !pCoeffs)
		return ippStsNullPtrErr;
	
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	/* for inverse scan */

#ifdef PRINT_REF_INFO
	{
			dump("ippiDecodeCoeffsIntra_MPEG4_1u16s		with rvlcFlag(0,1): ",rvlcFlag);
			dump("ippiDecodeCoeffsIntra_MPEG4_1u16s		with scan(0,1,2): ",scan);
	}
#endif

	memset(pCoeffs+noDCFlag, 0, (64-noDCFlag)*sizeof(Ipp16s));

	i = noDCFlag;
	if(!rvlcFlag)	/* vlc decoding intra, and inverse scan */
	{	
		do
		{
			run_level = VlcDecodeIntraTCOEF(ppBitStream,pBitOffset);

			if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
			{
#ifdef _DEBUG
				printf ("Too Much AC Coesffs in Intra block!");
#endif
				return ippStsVLCErr;
			}
			if (run_level.sign)		/* == 1 */
			{
				pCoeffs[pZigzag[i]] = -run_level.level;
			}
			else
			{
				pCoeffs[pZigzag[i]] = run_level.level;
			}

			i++;
		}
		while (!run_level.last);

		*pIndxLastNonZero = i-1;
	}
	else		/* RVLC */
	{
		do
		{
			run_level = RvlcDecodeIntraTCOEF(ppBitStream, pBitOffset);

			if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
			{
#ifdef _DEBUG
				printf ("Too Much AC Coesffs in Intra block!");
#endif
				return ippStsVLCErr;
			}
			if (run_level.sign)		/* == 1 */
			{
				pCoeffs[pZigzag[i]] = -run_level.level;
			}
			else
			{
				pCoeffs[pZigzag[i]] = run_level.level;
			}
			i++;
		}
		while (!run_level.last);

		*pIndxLastNonZero = i-1;
	}

	return ippStsNoErr;
}


/*
ReconstructCoeffsInter_MPEG4
Decodes DCT coefficients, performs inverse scan and
inverse quantization for inter coded block.
*/
IppStatus __STDCALL 
ippiReconstructCoeffsInter_MPEG4_1u16s_c(			
    Ipp8u**                            ppBitStream,
    int*                               pBitOffset,
    Ipp16s*                            pCoeffs,
    int*                               pIndxLastNonZero,
    int                                rvlcFlag,
    int                                scan,
    const IppiQuantInvInterSpec_MPEG4* pQuantInvInterSpec,
    int                                QP)
{
	Ipp32s              i;
	int sum;
	Tcoef               run_level;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[scan+1];
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);


	k_IppiQuantInvInterSpec_MPEG4 * pQuant = (k_IppiQuantInvInterSpec_MPEG4*)pQuantInvInterSpec;

	if (!ppBitStream || !pBitOffset || !pCoeffs || !pQuantInvInterSpec)
		return ippStsNullPtrErr;
	
	if (QP < 1 || QP > pQuant->quant_max)
		return ippStsQPErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	/* for inverse scan */
	sum=0;

#ifdef PRINT_REF_INFO
		{
			dump("ippiReconstructCoeffsInter_MPEG4_1u16s		with rvlcFlag(0,1): ",rvlcFlag);
			dump("ippiReconstructCoeffsInter_MPEG4_1u16s		with quant_type(0,1): ",pQuant->bUseMat);
			dump("ippiReconstructCoeffsInter_MPEG4_1u16s		with scan(0,1,2): ",scan);
		}
#endif

	memset(pCoeffs,0,64*sizeof(Ipp16s));

	i = 0;
	if(!rvlcFlag)	/* vlc decoding inter, and inverse scan , inverse quant*/
	{
		if (pQuant->bUseMat)	/*1 == quant_type */
		{
			do
			{
				run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,0, 0);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!\n");
#endif		
					return ippStsVLCErr;
				}

				tmpCoef = (((run_level.level<<1) + 1) * pQuant->matrix[pZigzag[i]] * QP) >> 4;	/* /16 */

				if (run_level.sign) 
					tmpCoef = -tmpCoef;

				tmpCoef = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += tmpCoef;
				pCoeffs[pZigzag[i]] = tmpCoef;

				/*pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += pCoeffs[pZigzag[i]];*/

				i++;
			}while (!run_level.last);
		}
		else		/*0 == quant_type */
		{
			do
			{
				run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,0, 0);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!\n");
#endif		
					return ippStsVLCErr;
				}
				tmpCoef = QP_1 * run_level.level + QP_2;

				if (run_level.sign)
					tmpCoef = -tmpCoef;

				pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);

				i++;	
			}while (!run_level.last);
		}
		*pIndxLastNonZero = i-1;
	}
	else		/*RVLC*/
	{
		if (pQuant->bUseMat)	/*1 == quant_type */
		{
			do
			{
				run_level = RvlcDecodeInterTCOEF(ppBitStream,pBitOffset);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!");
#endif		
					return ippStsVLCErr;
				}
				tmpCoef = (((run_level.level<<1) + 1) * pQuant->matrix[pZigzag[i]] * QP) >> 4;	/* /16 */

				if (run_level.sign) 
					tmpCoef = -tmpCoef;

				tmpCoef = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += tmpCoef;
				pCoeffs[pZigzag[i]] = tmpCoef;

				/*pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += pCoeffs[pZigzag[i]];*/

				i++;
			}while (!run_level.last);
		}
		else	/*0 == quant_type */
		{
			do
			{
				run_level = RvlcDecodeInterTCOEF(ppBitStream,pBitOffset);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!");
#endif		
					return ippStsVLCErr;
				}

				tmpCoef = QP_1 * run_level.level + QP_2;

				if (run_level.sign)
					tmpCoef = -tmpCoef;

				pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);

				i++;
			}while (!run_level.last);
		}
		*pIndxLastNonZero = i-1;
	}

	/* inverse quantization */
	if (pQuant->bUseMat)	 /*1 == quant_type*/
	{
		/*mismatch control*/
		if ((sum & 0x1) == 0) 
			pCoeffs[63] += ((pCoeffs[63] & 0x1)? -1:1); 
	}
	return ippStsNoErr;
}

#if 0
/*----------------------------------------------------------------------------/
 12 functions more which are used in this decoder sample
----------------------------------------------------------------------------*/
IppStatus __STDCALL 
ippiReconstructCoeffsIntra_H263_1u16s_flv(
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     cbp,
  int     QP,
  int     advIntraFlag,
  int     scan,
  int     modQuantFlag,
  int	  h263_flv)
{
	Ipp32s              i;
	Tcoef               run_level;
	Ipp32u				DC_coeff;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[scan+1];
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);

	if (!ppBitStream || !pBitOffset || !pCoef)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	
	if (QP < 1 || QP > 31)
		return ippStsQPErr;

#ifdef PRINT_REF_INFO
		{
			dump("ippiReconstructCoeffsIntra_H263_1u16s_flv		with cbp(0,!0): ",cbp);
			dump("ippiReconstructCoeffsIntra_H263_1u16s_flv		with scan(0,1,2): ",scan);
		}
#endif

	/* in this Intel sample, 
	  mp4decvop.c line # 791  and  mp4dec.h line # 410 use this function
	  and the modQuantFlag, advIntraFlag are both set with 0 obviously.
	  I think modQuantFlag and advIntraFlag are used in H.263, not mpeg4
	  short video header mode*/
	if (modQuantFlag || advIntraFlag)
	{
#ifdef _DEBUG
		printf("modQuantFlag or advIntraFlag is not used in short video header mode of mpeg4\n");
#endif 
		return ippStsErr;
	}

	/*DC_coeff = k_mp4_GetBits(ppBitStream, pBitOffset, 8);*/
	DC_coeff = k_mp4_GetBit8(ppBitStream, pBitOffset);

	if (DC_coeff == 128 || DC_coeff == 0) 
	{
#ifdef _DEBUG
		printf ("short header: Illegal DC coeff: 0 or 128\n");
#endif
		*pIndxLastNonZero = -1;
		return ippStsVLCErr;
	}
	if (DC_coeff == 255)
		DC_coeff = 128;

	pCoef[0] = DC_coeff << 3;	/* DC_coeff * 8 */
	pCoef[0] = CLIP_R(pCoef[0],2047,-2048);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

	if (0 == cbp)	/* only return the intra DC */
	{
		*pIndxLastNonZero = 0;
		return ippStsNoErr;
	}

	/* other intra TCoeffs */
	memset(pCoef+1,0,63*sizeof(Ipp16s));

	i = 1;
	do
	{
		run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,1,h263_flv);

		if ((i+= run_level.run) > 63)
		{
#ifdef _DEBUG
			printf ("Too Much AC Coesffs in short header Intra block!\n");
#endif
			*pIndxLastNonZero = -1;
			return ippStsVLCErr;
		}

		/*	inverse scan & inverse quant */
		tmpCoef = QP_1 * run_level.level + QP_2;

		if (run_level.sign)
			tmpCoef = -tmpCoef;

		pCoef[pZigzag[i]] = CLIP_R(tmpCoef,2047,-2048);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

		i++;
	}while (!run_level.last);

	*pIndxLastNonZero = i-1;

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiReconstructCoeffsInter_H263_1u16s_flv(
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     QP,
  int     modQuantFlag,
  int	  h263_flv)
{
	Ipp32s              i;
	Tcoef               run_level;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[1];
	const int			QP_1 = QP << 1;
	const int			QP_2 = (QP & 1) ? QP : (QP - 1);

	if (!ppBitStream || !pBitOffset || !pCoef)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	
	if (QP < 1 || QP > 31)
		return ippStsQPErr;

#ifdef PRINT_REF_INFO
	dump("ippiReconstructCoeffsInter_H263_1u16s_flv", -1);
#endif

	/* in this Intel sample, 
		mp4dec.h line #428 line #503 and mp4decvop.c line #816
	  the modQuantFlag is set with 0 obviously.
	  I think modQuantFlag are used in H.263, not mpeg4
	  short video header mode*/
	if (modQuantFlag)
	{
#ifdef _DEBUG
		printf("modQuantFlag is not used in short video header mode of mpeg4\n");
#endif 
		return ippStsErr;
	}

	memset(pCoef,0,64*sizeof(Ipp16s));

	i = 0;
	do
	{
		run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,1, h263_flv);

		if ((i+= run_level.run) > 63)
		{
#ifdef _DEBUG
			printf ("Too Much AC Coesffs in short header Inter block!\n");
#endif
			*pIndxLastNonZero = -1;
			return ippStsVLCErr;
		}

		tmpCoef = QP_1 * run_level.level + QP_2;

		if (run_level.sign)
			tmpCoef = -tmpCoef;

		pCoef[pZigzag[i]] = CLIP_R(tmpCoef,2047,-2048);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

		i++;
	}while (!run_level.last);

	*pIndxLastNonZero = i-1;

	return ippStsNoErr;
}

#endif

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
ippiCopy8x4HP_8u_C1R_c(
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

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy8x4HP_8u_C1R_c		with acc(0,1,2,3): ",acc);
		dump("ippiCopy8x4HP_8u_C1R_c		with rounding(0,1): ",rounding);
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
#ifdef WIN32
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
	register int round = 1 - rounding;
	Ipp8u *pS, *pD;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

#ifdef PRINT_REF_INFO
	{
		dump("ippiCopy8x8HP_8u_C1R_c		with acc(0,1,2,3): ",acc);
		dump("ippiCopy8x8HP_8u_C1R_c		with rounding(0,1): ",rounding);
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
		for (i = 8; i > 0; i--)
		{
#ifdef WIN32
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*(p1++) = *(p2++);
			*p1 = *p2;
#else
			memcpy(pD,pS,8);
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
#endif
			pD += dstStep;
			pS += srcStep;
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
	register int round = 1- rounding;
	Ipp8u *pS, *pD;
	
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
#ifdef WIN32
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
 this function does the way just same as ippiCopy8x8HP_8u_C1R_c
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
	register int round = 1- rounding;
	Ipp8u *pS, *pD;
	
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
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (i = 16; i > 0; i--)
		{
#ifdef WIN32
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*(p1++) = *(p2++);
			*p1 = *p2;
#else
			memcpy(pD,pS,16);
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
#endif
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 1)
	{
		for (i = 16; i > 0; i--)
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
	Ipp8u tmp[64];
	Ipp8u *pD, *pT;
	Ipp16s *pS1;
	int i,halfstep;
	ippiCopy8x8HP_8u_C1R_c(pSrc2,src2Step,tmp,8,acc,rounding);
	
	halfstep = src1Step>>1;

	pD = pDst;
	pS1 = (Ipp16s*)pSrc1;
	pT = tmp;
	for (i = 8; i > 0; i--)
	{
		pD[0] = CLIP(pT[0] + pS1[0]);
		pD[1] = CLIP(pT[1] + pS1[1]);
		pD[2] = CLIP(pT[2] + pS1[2]);
		pD[3] = CLIP(pT[3] + pS1[3]);
		pD[4] = CLIP(pT[4] + pS1[4]);
		pD[5] = CLIP(pT[5] + pS1[5]);
		pD[6] = CLIP(pT[6] + pS1[6]);
		pD[7] = CLIP(pT[7] + pS1[7]);

		pS1 += halfstep;
		pT += 8;
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
