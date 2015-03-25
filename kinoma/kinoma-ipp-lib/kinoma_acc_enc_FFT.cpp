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
#ifdef __KINOMA_IPP__

#include <math.h>
#include <assert.h>

#include "ipps.h"
#include "ippdc.h"
#include "ippac.h"

#include "kinoma_aac_defines.h"
#include "kinoma_ipp_common.h"
#include "kinoma_ipp_lib.h"

IppStatus (__STDCALL *ippsFFTInitAlloc_R_16s_universal)				( IppsFFTSpec_R_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )=NULL;
IppStatus (__STDCALL *ippsFFTFree_R_16s_universal) 					( IppsFFTSpec_R_16s*  pFFTSpec )=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_R_16s_universal) 			(const IppsFFTSpec_R_16s*  pFFTSpec, int* pSize )=NULL;
IppStatus (__STDCALL *ippsFFTFwd_RToCCS_16s_Sfs_universal)			(const Ipp16s* pSrc,  Ipp16s* pDst,  const IppsFFTSpec_R_16s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )=NULL;

void AAC_ippsFFTInitAlloc(double **cosTbl, double **sinTbl, int **reorderTbl,  double **ppbuff, int simpleLen, int order)
{
	int halfsize = simpleLen >> 1;  // simpleLen must equal == N/4 : 64 or 512
	int i;
	double theta;
	double *LcosTbl,  *LsinTbl, *Lpbuff;
	int *LreorderTbl;

	LcosTbl = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);
	LsinTbl = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);
	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)simpleLen;
		LcosTbl[i] = cos(theta);
		LsinTbl[i] = -sin(theta);
	}

	/*bit reverse table*/
	LreorderTbl = (int*)ippsMalloc_8u_c(sizeof(int)*simpleLen);
	for (i = 0; i < simpleLen; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		LreorderTbl[i] = reversed;
	}

	Lpbuff  = (double*)ippsMalloc_8u_c(sizeof(double)*simpleLen*8);  // Workaroud

	*cosTbl = LcosTbl; *sinTbl = LsinTbl; *reorderTbl = LreorderTbl; *ppbuff = Lpbuff;
}

#define _32BIT_FFT_

#ifndef _32BIT_FFT_
IppStatus __STDCALL ippsFFTInitAlloc_C_16sc_c(IppsFFTSpec_C_16sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )
{
	k_IppsFFTSpec_C_16sc *pkSpec;
	int buffSize = (1<<(order+2)) * 4 + 16;
	
	if (!ppFFTSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;
	
	pkSpec = (k_IppsFFTSpec_C_16sc*)ippsMalloc_8u_c(sizeof(k_IppsFFTSpec_C_16sc));
	if (!pkSpec)
		return ippStsMemAllocErr;

	pkSpec->sig = 0x3;							/*means Real fft*/ //0x003
	pkSpec->order = order;
	pkSpec->order1 = pkSpec->order2 = pkSpec->order3 = 0;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = buffSize;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 1;									/*means this is InitAlloc func, not Init*/
	//pkSpec->startAddr = 0;

	// 
	pkSpec->len = 1 << (order+2);							/* 2^order will be the input sample length */

	/*sin and cos table*/
	AAC_ippsFFTInitAlloc(&pkSpec->cos, &pkSpec->sin, &pkSpec->ReverseTbl, &pkSpec->pbuff, pkSpec->len/4, pkSpec->order);

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
	else if (flag == IPP_FFT_DIV_FWD_BY_N)
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	*ppFFTSpec = (IppsFFTSpec_C_16sc*)pkSpec;

	return ippStsNoErr;
}


IppStatus __STDCALL ippsFFTFree_C_16sc_c( IppsFFTSpec_C_16sc* pFFTSpec )
{
	k_IppsFFTSpec_C_16sc *pkSpec = (k_IppsFFTSpec_C_16sc*)pFFTSpec;
	
	if (!pFFTSpec)
		return ippStsNullPtrErr;

	if (pkSpec->alloc)
	{
		ippsFree_c(pkSpec->sin);
		ippsFree_c(pkSpec->cos);
		ippsFree_c(pkSpec->ReverseTbl);
		ippsFree_c(pkSpec->pbuff);
		ippsFree_c(pkSpec);
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippsFFTGetBufSize_C_16sc_c (const IppsFFTSpec_C_16s*  pFFTSpec, int* pSize )
{
	k_IppsFFTSpec_C_16s *pkSpec = (k_IppsFFTSpec_C_16s*)pFFTSpec;

	if (!pFFTSpec)
		return ippStsNullPtrErr;

	*pSize = pkSpec->sizeWorkBuf + 16;

	return ippStsNoErr;
}




IppStatus __STDCALL ippsFFTFwd_CToC_16sc_Sfs_c (const Ipp16sc* pSrc, Ipp16sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )
{
	k_IppsFFTSpec_C_16sc * pkSpec = (k_IppsFFTSpec_C_16sc*)pFFTSpec;

	int i,j;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	double *xr, *xi, tmp;
	int  len = pkSpec->len/4;

	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	xr = pkSpec->pbuff;
	xi = (pkSpec->pbuff + len);
	for (i = 0; i < len; i++)
	{
		xr[i] = pSrc[i].re;
		xi[i] = pSrc[i].im;
	}

	/*bit reverse*/
	// Do two for Complex
	for (i = 0; i < len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*fwd transform*/
	estep = size = len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				long double v2r, v2i;

				v2r = xr[x2] * pkSpec->cos[exp] - xi[x2] * pkSpec->sin[exp];
				v2i = xr[x2] * pkSpec->sin[exp] + xi[x2] * pkSpec->cos[exp];

				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	if (scaleFactor)
		rounding = (1<< (scaleFactor -1 ))+0.5;
	else
		rounding = 0.5;

	if (pkSpec->flagFwd)
	{
		for (i = 0; i < len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp16s)(((__int64)((xr[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp16s)(((__int64)((xr[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp16s)(((__int64)((xi[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp16s)(((__int64)((xi[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp16s)(((__int64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp16s)(((__int64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp16s)(((__int64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp16s)(((__int64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
	return ippStsNoErr;
}
#else
#if 0	//***bnie: dupliated???
IppStatus __STDCALL ippsFFTInitAlloc_C_32sc_c_nono(IppsFFTSpec_C_32sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )
{
	k_IppsFFTSpec_C_32sc *pkSpec;
	
	if (!ppFFTSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;
	
	pkSpec = (k_IppsFFTSpec_C_32sc*)ippsMalloc_8u_c(sizeof(k_IppsFFTSpec_C_32sc));
	if (!pkSpec)
		return ippStsMemAllocErr;

	pkSpec->sig = REAL_SIG;							/*means Real fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = (1<<(order+2)) * 8 + 32;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 1;									/*means this is InitAlloc func, not Init*/
	pkSpec->len = 1 << (order+2);							/* 2^order will be the input sample length */


	/*sin and cos table*/
	AAC_ippsFFTInitAlloc(&pkSpec->cos, &pkSpec->sin, &pkSpec->ReverseTbl, &pkSpec->pbuff, pkSpec->len/4, pkSpec->order);

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
	else if (flag == IPP_FFT_DIV_FWD_BY_N)
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	*ppFFTSpec = (IppsFFTSpec_C_32sc*)pkSpec;

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTFree_C_32sc_c(IppsFFTSpec_C_32sc* pFFTSpec)
{
	k_IppsFFTSpec_C_32sc *pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;
	
	if (!pFFTSpec)
		return ippStsNullPtrErr;

	if (pkSpec->alloc)
	{
		ippsFree_c(pkSpec->sin);
		ippsFree_c(pkSpec->cos);
		ippsFree_c(pkSpec->ReverseTbl);
		ippsFree_c(pkSpec->pbuff);
		ippsFree_c(pkSpec);
	}

	return ippStsNoErr;
}
IppStatus __STDCALL ippsFFTGetBufSize_C_32sc_c(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize)
{
	k_IppsFFTSpec_C_32sc *pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	if (!pFFTSpec)
		return ippStsNullPtrErr;


	*pSize = ((k_IppsFFTSpec_C_32sc*)pFFTSpec)->sizeWorkBuf + 16;

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTFwd_CToC_32sc_Sfs_c (const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer )
{
	k_IppsFFTSpec_C_16sc * pkSpec = (k_IppsFFTSpec_C_16sc*)pFFTSpec;

	int i,j;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	double *xr, *xi, tmp;
	int  len = pkSpec->len/4;

	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	xr = pkSpec->pbuff;
	xi = (pkSpec->pbuff + len);
	for (i = 0; i < len; i++)
	{
		xr[i] = pSrc[i].re;
		xi[i] = pSrc[i].im;
	}

	/*bit reverse*/
	// Do two for Complex
	for (i = 0; i < len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*fwd transform*/
	estep = size = len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				long double v2r, v2i;

				v2r = xr[x2] * pkSpec->cos[exp] - xi[x2] * pkSpec->sin[exp];
				v2i = xr[x2] * pkSpec->sin[exp] + xi[x2] * pkSpec->cos[exp];

				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	if (scaleFactor)
		rounding = (1<< (scaleFactor -1 ))+0.5;
	else
		rounding = 0.5;

	if (pkSpec->flagFwd)
	{
		for (i = 0; i < len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp16s)(((__int64)((xr[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp16s)(((__int64)((xr[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp16s)(((__int64)((xi[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp16s)(((__int64)((xi[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp16s)(((__int64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp16s)(((__int64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp16s)(((__int64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp16s)(((__int64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
	return ippStsNoErr;
}



#endif

#endif

IppStatus __STDCALL ippsFFTInitAlloc_R_16s_c( IppsFFTSpec_R_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )
{
	k_IppsFFTSpec_R_16s *pkSpec;
	int buffSize = (1<<(order+2)) * 2 + 24;   // Pleaase note we need +2 here even order has NOT substract 2 by its caller, just because we do 2048 FFT(Not N/4)
	
	if (!ppFFTSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;
	
	pkSpec = (k_IppsFFTSpec_R_16s*)ippsMalloc_8u_c(sizeof(k_IppsFFTSpec_R_16s));
	if (!pkSpec)
		return ippStsMemAllocErr;

	pkSpec->sig = 0x4c4d5365;							/*means Real fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = buffSize;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 1;									/*means this is InitAlloc func, not Init*/
	//pkSpec->startAddr = 0;

	// 
	pkSpec->len = 1 << (order+2);							/* 2^order will be the input sample length */

	/*sin and cos table*/
	AAC_ippsFFTInitAlloc(&pkSpec->cos, &pkSpec->sin, &pkSpec->ReverseTbl, &pkSpec->pbuff, pkSpec->len/4, pkSpec->order);

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}/*
	else if (flag == IPP_FFT_DIV_FWD_BY_N)
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}*/

	*ppFFTSpec = (IppsFFTSpec_R_16s*)pkSpec;


	return ippStsNoErr;

}





IppStatus __STDCALL ippsFFTFree_R_16s_c ( IppsFFTSpec_R_16s*  pFFTSpec )
{
	k_IppsFFTSpec_R_16s *pkSpec = (k_IppsFFTSpec_R_16s*)pFFTSpec;
	
	if (!pFFTSpec)
		return ippStsNullPtrErr;

	if (pkSpec->alloc)
	{
		ippsFree_c(pkSpec->sin);
		ippsFree_c(pkSpec->cos);
		ippsFree_c(pkSpec->ReverseTbl);
		ippsFree_c(pkSpec->pbuff);
		ippsFree_c(pkSpec);
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTGetBufSize_R_16s_c (const IppsFFTSpec_R_16s*  pFFTSpec, int* pSize )
{
	k_IppsFFTSpec_R_16s *pkSpec = (k_IppsFFTSpec_R_16s*)pFFTSpec;

	if (!pFFTSpec)
		return ippStsNullPtrErr;

	*pSize = pkSpec->sizeWorkBuf + 16;

	return ippStsNoErr;
}
IppStatus __STDCALL ippsFFTFwd_RToCCS_16s_Sfs_c(const Ipp16s* pSrc,  Ipp16s* pDst,  const IppsFFTSpec_R_16s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )
{
	k_IppsFFTSpec_R_16s * pkSpec = (k_IppsFFTSpec_R_16s*)pFFTSpec;

	int i,j,k;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	double *xr, *xi, tmp;

	int  len = pkSpec->len/4;

	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	xr = pkSpec->pbuff;
	xi = (pkSpec->pbuff + len);
	
	// Here has one question? R?
	for (i = 0, k=0; i < len; i++)
	{
		xr[k] = pSrc[i];
		xi[k++] = 0; // Just include real data
	}

	/*bit reverse*/
	for (i = 0; i < len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*fwd transform*/
	estep = size = len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				long double v2r, v2i;

				v2r = xr[x2] * pkSpec->cos[exp] - xi[x2] * pkSpec->sin[exp];
				v2i = xr[x2] * pkSpec->sin[exp] + xi[x2] * pkSpec->cos[exp];

				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	if (scaleFactor)
		rounding = (1<< (scaleFactor -1 ))+0.5;
	else
		rounding = 0.5;


	{
		int c;
		if (xr[i] < 0)
			pDst[0] = (Ipp16s)(((__int64)(xr[0] - rounding)) >> scaleFactor);
		else
			pDst[0] = (Ipp16s)(((__int64)(xr[0] + rounding)) >> scaleFactor);

		pDst[1] = 0;
		c =2;
		for (i = 1; i < len/2; i++)
		{
			if (xr[i] < 0)
				pDst[c++] = (Ipp16s)(((__int64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp16s)(((__int64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[c++] = (Ipp16s)(((__int64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp16s)(((__int64)(xi[i] + rounding)) >> scaleFactor);
		}

		if (xr[i] < 0)
			pDst[c++] = (Ipp16s)(((__int64)(xr[len/2] - rounding)) >> scaleFactor);
		else
			pDst[c++] = (Ipp16s)(((__int64)(xr[len/2] + rounding)) >> scaleFactor);

		pDst[c] = 0;

	}

	return ippStsNoErr;
}


#endif
