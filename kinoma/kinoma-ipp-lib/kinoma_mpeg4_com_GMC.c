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
#include "kinoma_ipp_common.h"

#ifndef DROP_GMC

IppStatus (__STDCALL *ippiWarpGetSize_MPEG4_universal)						( int*  pSpecSize)=NULL;
IppStatus (__STDCALL *ippiChangeSpriteBrightness_MPEG4_8u_C1IR_universal) 	(Ipp8u*  pSrcDst, int srcDstStep, int width, int height, int brightnessChangeFactor)=NULL;
IppStatus (__STDCALL *ippiCalcGlobalMV_MPEG4_universal)						(int xOffset, int yOffset, IppMotionVector* pGMV,const IppiWarpSpec_MPEG4*  pSpec)=NULL;
IppStatus (__STDCALL *ippiWarpChroma_MPEG4_8u_P2R_universal)				(const Ipp8u* pSrcCb, int srcStepCb,const Ipp8u* pSrcCr,int srcStepCr,Ipp8u* pDstCb, int dstStepCb,Ipp8u* pDstCr,int dstStepCr, const IppiRect* dstRect, const IppiWarpSpec_MPEG4* pSpec)=NULL;    
IppStatus (__STDCALL *ippiWarpLuma_MPEG4_8u_C1R_universal)					(const Ipp8u* pSrcY,int srcStepY, Ipp8u* pDstY, int dstStepY, const IppiRect* dstRect,const IppiWarpSpec_MPEG4* pSpec)=NULL;
IppStatus (__STDCALL *ippiWarpInit_MPEG4_universal)							(IppiWarpSpec_MPEG4* pSpec, const int* pDU, const int* pDV, int numWarpingPoints, int spriteType, int warpingAccuracy, int roundingType, int quarterSample, int fcode, const IppiRect* spriteRect, const IppiRect* vopRect)=NULL;


/**************************************************************
* my IppiWarpSpec_MPEG4 structure							*
* the size of Intel's is 384 bytes							*
**************************************************************/
typedef struct
{
	/* reference points all */
	int vop_i0,vop_j0,vop_i1,vop_j1,vop_i2,vop_j2,vop_i3,vop_j3;
	int spr_i0,spr_j0,spr_i1,spr_j1,spr_i2,spr_j2,spr_i3,spr_j3;
	int spr_vi1, spr_vj1, spr_vi2, spr_vj2;

	/* Width, height, and virtual Width, height */
	int W, H, VW, VH;  /* VW, VH is for virtual sprite points */

	int minYW,maxYW,minCW,maxCW;	/* clip for warping points */
	int minYH,maxYH,minCH,maxCH;

	/* power for W, H, R*/
	int alpha, beta,rou;
	
	/* s*/
	int s,pwr_s,ps_round,s_pwr,ps_p, mv_scaler, mv_half, mv_denom;
	
	int min, max;

	int ic,jc; /* 2*vop_i0 - 1,    2*vop_j0 - 1*/

	int pwr_aArM1,aAr,aArA2;	/* 2pow(alph+rou-1),  (alpha+rou)  (alpha+rou+2) */

	/* for sprite_warping_points == 0  chroma*/

	/* for sprite_warping_points == 1 luma*/

	/* for sprite_warping_points == 1 and static  chroma*/
	int wpt1_1, wpt1_2;
	/* for sprite_warping_points == 1 and GMC  chroma*/

	/* for sprite_warping_points == 2 */
	int wpt2_1, wpt2_2, wpt2_3;	/* (-r*spr_i0+spr_vi1),  (r*spr_j0-spr_vj1), (-r*spr_j0+spr_vj1), */

	/* for sprite_warping_points == 2 chroma*/
	int wpt2_4;					/* (2*VW*r*spr_i0 -  16*VW + 2pow(alpha+rou+1))  */
	int wpt2_5;					/* (2*VW*r*spr_j0 -  16*VW + 2pow(alpha+rou+1))  */

	/* for sprite_warping_points == 3 */
	int wpt3_1, wpt3_2;		/* (-r*spr_i0+spr_vi1), (-r*spr_i0_spr_vi2)*2pow(alpha-beta)*/
	int wpt3_3, wpt3_4;		/* (-r*spr_j0+spr_vj1), (-r*spr_j0_spr_vj2)*2pow(alpha-beta)*/

	/* for sprite_warping_points == 3 chroma*/
	int wpt3_5;			/* (2*VW*r*spr_i0-16*VW+2pow(alpha+rou+1)) */
	int wpt3_6;			/* (2*VW*r*spr_j0-16*VW+2pow(alpha+rou+1)) */

	/* for sprite_warping_points == 4 */
	int g;		/* ((spr_i0-spr_i1-spr_i2+spr_i3)(spr_j2-spr_j3) - (spr_i2-spr_i3)(spr_j0-spr_j1-spr_j2+spr_j3)) *H  */
	int h;		/* ((spr_i1-spr_i3)(spr_j0-spr_j1-spr_j2+spr_j3) - (spr_i0-spr_i1-spr_i2+spr_i3)(spr_j1-spr_j3)) *W  */
	int D;		/* (spr_i1-spr_i3) (spr_j2-spr_j3) - (spr_i2-spr_i3)(spr_j1-spr_j3) */
	int a;		/* D(spr_i1 - spr_i0) H + g*spr_i1 */
	int b;		/* D(spr_i2 - spr_i0) W + h*spr_i2 */
	int c;		/* D*spr_i0*W*H  */
	int d;		/* D(spr_j1-spr_j0) H + g*spr_j1  */
	int e;		/* D(spr_j2-spr_j0) W + h*spr_j2  */
	int f;		/* D*spr_j0*W*H */


	/*	input data */
    int                 numWarpingPoints;
    int                 spriteType;

}k_IppiWarpSpec_MPEG4;



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
				+ rj * ((pWarp->s - ri) * pSrcY[pos+srcStepY] + ri * pSrcY[pos+1+srcStepY]) + pWarp->ps_round)>>pWarp->ps_p; /pWarp->pwr_s;*/
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
				+ rj * ((pWarp->s - ri) * pSrcCb[posb+srcStepCb] + ri * pSrcCb[posb+1+srcStepCb]) + pWarp->ps_round) >> pWarp->ps_p; /pWarp->pwr_s;*/

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

#endif
