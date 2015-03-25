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
#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"


#ifdef DUPLICATE_32_FFT
IppStatus (__STDCALL *ippsFFTInitAlloc_C_32sc_universal)(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint)=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_C_32sc_universal)(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize)=NULL;
IppStatus (__STDCALL *ippsFFTFree_C_32sc_universal)		(IppsFFTSpec_C_32sc* pFFTSpec)=NULL;
IppStatus (__STDCALL *ippsFFTFwd_CToC_32sc_Sfs_universal)(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;

IppStatus (__STDCALL *ippsFFTInv_CToC_32s_Sfs_universal)(const Ipp32s* pSrcRe, const Ipp32s* pSrcIm, Ipp32s* pDstRe, Ipp32s* pDstIm, const IppsFFTSpec_C_32s* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;
IppStatus (__STDCALL *ippsFFTInit_C_32s_universal)		(IppsFFTSpec_C_32s** ppFFTSpec, int order, int flag,IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)=NULL;

/* the following 6 functions exist in sbr code */
IppStatus (__STDCALL *ippsFFTGetSize_C_32sc_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf)=NULL;
IppStatus (__STDCALL *ippsFFTInit_C_32sc_universal)		(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)=NULL;
IppStatus (__STDCALL *ippsFFTInv_CToC_32sc_Sfs_universal)(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;
IppStatus (__STDCALL *ippsFFTGetSize_C_32s_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf)=NULL;

IppStatus (__STDCALL *ippsFFTFwd_RToPack_32s_Sfs_universal)			(const Ipp32s* pSrc, Ipp32s* pDst,   const IppsFFTSpec_R_32s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )=NULL;
IppStatus (__STDCALL *ippsFFTInv_PackToR_32s_Sfs_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, const IppsFFTSpec_R_32s* pFFTSpec,   int scaleFactor, Ipp8u* pBuffer )=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_R_32s_universal) 			(const IppsFFTSpec_R_32s*  pFFTSpec, int* pSize )=NULL;
IppStatus (__STDCALL *ippsFFTInitAlloc_R_32s_universal)				(IppsFFTSpec_R_32s** ppFFTSpec,int order, int flag, IppHintAlgorithm hint )=NULL;

#endif

//IppStatus (__STDCALL *ippsFFTInitAlloc_C_16s_universal)				( IppsFFTSpec_C_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )=NULL;
//IppStatus (__STDCALL *ippsFFTFree_C_16s_universal)					( IppsFFTSpec_C_16s*  pFFTSpec )=NULL;
//IppStatus (__STDCALL *ippsFFTFwd_CToC_16sc_Sfs_universal)			(const Ipp16sc* pSrc, Ipp16sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )=NULL;
//IppStatus (__STDCALL *ippsFFTInitAlloc_C_16sc_universal)				(IppsFFTSpec_C_16sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )=NULL;
//IppStatus (__STDCALL *ippsFFTGetBufSize_C_16sc_universal) 			(const IppsFFTSpec_C_16sc*  pFFTSpec, int* pSize )=NULL;
//IppStatus (__STDCALL *ippsFFTFree_C_16sc_universal)					( IppsFFTSpec_C_16sc* pFFTSpec )=NULL;


IppStatus (__STDCALL *ippsMDCTFwdGetBufSize_16s_universal)			(const IppsMDCTFwdSpec_16s *pMDCTSpec, int *pSize)=NULL;
IppStatus (__STDCALL *ippsMDCTFwdFree_16s_universal)					(IppsMDCTFwdSpec_16s* pMDCTSpec)=NULL;
IppStatus (__STDCALL *ippsMDCTFwdInitAlloc_16s_universal)			(IppsMDCTFwdSpec_16s ** ppMDCTSpec, int len)=NULL;
IppStatus (__STDCALL *ippsMDCTFwd_16s_Sfs_universal)					(const Ipp16s *pSrc, Ipp16s *pDst,const IppsMDCTFwdSpec_16s* pMDCTSpec,int scaleFactor, Ipp8u* pBuffer)=NULL;

#define KINOMA_FFT

#define	   COMPLEX_SIG	1280136035	//cSML
#define	   REAL_SIG		1280136036	//dSML
#define    VLC_SIG		1280136057	//ySML


/*
	Comments for version 1.3
	If we want to use 32bits FFT, we need define it: #define _32BIT_FFT_
*/
#define _32BIT_FFT_

IppStatus __STDCALL ippsFFTFwd_CToC_16sc_Sfs_c (const Ipp16sc* pSrc, Ipp16sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer );
IppStatus __STDCALL ippsFFTFwd_CToC_32sc_Sfs_c (const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer );

#define MALLOC_ALIGNED_BYTES 32
#define ALIGNED_PTR(ptr, bytes) \
  (ptr + ((bytes - ((Ipp32s)ptr & (bytes-1))) & (bytes-1)))

#define RESERVE_SIZE     MALLOC_ALIGNED_BYTES

#define RETURN_ALIGNED_MEM(ptr) ALIGNED_PTR(ptr, MALLOC_ALIGNED_BYTES)

/*******************************************************************/

#define MDCT_BUFSIZE_COUNT(FFTBufSize, MDCTBufSize)                   \
  MDCTBufSize = FFTBufSize;                                           \
  if (MDCTBufSize < ((len)*(int)sizeof(Ipp16sc)))                     \
      MDCTBufSize = ((len)*(int)sizeof(Ipp16sc));                     \
                                                                      \
  MDCTBufSize += ((len/4)*sizeof(Ipp16sc) + MALLOC_ALIGNED_BYTES);


#define IS_POWER_2(len)  (((len-1)&len)==0)
#define GOTO_RET(stat)   { res = stat; goto RET; }

// We need three type totally
typedef struct 
{
  int     sign;
  int     len;
  int     bufSize;
  int     order;
  int     alloc;
  Ipp16s *sincos;
  IppsFFTSpec_C_16sc *ctxFFT;
  int	dummy0;
} IppMDCTContext_16s;


// Please note this function: order is not subtract 2
// These three functions are subtract 2 already

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
#if 1	//***bnie: dupliated???
IppStatus __STDCALL ippsFFTInitAlloc_C_32sc_c(IppsFFTSpec_C_32sc** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )
{
	k_IppsFFTSpec_C_32sc *pkSpec;
	int buffSize = (1<<(order+2)) * 8 + 16;
	
	if (!ppFFTSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;
	
	pkSpec = (k_IppsFFTSpec_C_32sc*)ippsMalloc_8u_c(sizeof(k_IppsFFTSpec_C_32sc));
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

IppStatus __STDCALL ippsFFTFwd_CToC_32sc_Sfs_c (const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer )
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

/************************************************************************
 * MDCT related function
 *
 *
 ************************************************************************* */

static void AAC_deleteMdctCtx(IppMDCTContext_16s * pCtxMDCT)
{
  if (pCtxMDCT != 0) {
    if (pCtxMDCT->alloc != 0) {
      if (pCtxMDCT->ctxFFT != 0) {
#ifndef _32BIT_FFT_
        ippsFFTFree_C_16sc_c(pCtxMDCT->ctxFFT);
#else
        ippsFFTFree_C_32sc_c(pCtxMDCT->ctxFFT);
#endif
      }
      ippsFree_c(pCtxMDCT);
    }
  }
}

static IppStatus AAC_MDCTinitAlloc_16s(IppMDCTContext_16s  **ppCtxMDCT, int len)
{
  IppMDCTContext_16s *pCtxMDCT = 0;
  IppStatus          res;
  Ipp16s             *sincos;
  int                i, order = 0;
  int                stateSize, bufSize = 0, FFTBufSize = 0;

  stateSize = sizeof(IppMDCTContext_16s) + (len/2) * sizeof(Ipp16s) + MALLOC_ALIGNED_BYTES;
  pCtxMDCT = (IppMDCTContext_16s*)ippsMalloc_8u_c(stateSize);
  if (pCtxMDCT == 0) {
    return ippStsMemAllocErr;
  }
  ippsZero_8u_c((Ipp8u*)pCtxMDCT, sizeof(IppMDCTContext_16s));

  pCtxMDCT->alloc = 1;

  sincos = (Ipp16s*)((Ipp8u*)pCtxMDCT + sizeof(IppMDCTContext_16s));
  sincos = (Ipp16s*)ALIGNED_PTR(sincos, MALLOC_ALIGNED_BYTES);
  pCtxMDCT->sincos = sincos;
  pCtxMDCT->len = len;

  if (IS_POWER_2(len)) {
    for (i = 1; i < len/4; i *= 2) {
      order++;
    }

#ifndef _32BIT_FFT_
    res = ippsFFTInitAlloc_C_16sc_c(&pCtxMDCT->ctxFFT, order/*Already = order-2*/, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone);
#else
    res = ippsFFTInitAlloc_C_32sc_c(&pCtxMDCT->ctxFFT, order/*Already = order-2*/, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone);
#endif
	if (res != ippStsNoErr) {
      GOTO_RET(res);
    }

#ifndef _32BIT_FFT_
    res = ippsFFTGetBufSize_C_16sc_c(pCtxMDCT->ctxFFT, &FFTBufSize);
#else
    res = ippsFFTGetBufSize_C_32sc_c(pCtxMDCT->ctxFFT, &FFTBufSize);
#endif
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }

  }

  MDCT_BUFSIZE_COUNT(FFTBufSize, bufSize)
  pCtxMDCT->bufSize = bufSize*2;  // WWD changed here in 20070422 for 32bits operation in MDCt;
  pCtxMDCT->order = order;
  pCtxMDCT->sign = 0x39;
  pCtxMDCT->dummy0 = 0xbaadf00d;

  *ppCtxMDCT = pCtxMDCT;
  return (ippStsNoErr);

RET:
  AAC_deleteMdctCtx(pCtxMDCT);
  return (res);
}

static void AAC_FwdFillSinCosBuf_16s(Ipp16s *sincos,Ipp32s len)
{
  double ang;
  int    i;

  ang = IPP_2PI/len;
  for (i = 0; i < len/4; i++) {

    sincos[2*i  ] = (Ipp32s)(sin(ang * (i+0.125)) * 16384 + 0.5);    /* 16384 = 2^14 */
    sincos[2*i+1] = (Ipp32s)(cos(ang * (i+0.125)) * 16384 + 0.5);

  }
}

// Above function can produce same result as ipp functions
// Pre-process: This function is scaled 2^13
#ifndef _32BIT_FFT_
static void AAC_MDCTFwdPreProc_16s(Ipp16s  *pSrc,   Ipp16sc *y,     int     n,   int     scalef,    Ipp16s  *pSinCos)
{
  int     n4    = n / 4;
  Ipp32s  round = 0;
  int     i;
  Ipp32s  re, im, yre, yim;

  if (scalef < 16) {
    round = ((Ipp32s)1) << (15 - scalef);
  }

  for (i = 0; i < (n4+1)/2; i++) {
    re = (Ipp32s)((-pSrc[n-n4+2*i]) - pSrc[n-n4-1-2*i]);
    im = (Ipp32s)((pSrc[n4-1-2*i]) - pSrc[n4+2*i]);

	yre = (Ipp32s)(re * pSinCos[2*i+1] + im * pSinCos[2*i] );
	yim = (Ipp32s)(im * pSinCos[2*i+1] - re * pSinCos[2*i] );

	if(yre >=0)
		yre = (Ipp16s)((yre + round) >> (16 - scalef));
	else
	{
		yre = (Ipp16s)(((-yre - round) >> (16 - scalef)) * -1);
	}

	if(yim >=0)
		yim = (Ipp16s)((yim + round) >> (16 - scalef));
	else
	{
		yim = (Ipp16s)(((-yim - round) >> (16 - scalef)) * -1);
	}

	//yre = (Ipp32s)((re * pSinCos[2*i+1] + im * pSinCos[2*i] + round) >> (16 - scalef));
	//yim = (Ipp32s)((im * pSinCos[2*i+1] - re * pSinCos[2*i] + round) >> (16 - scalef));

	//assert(yre<0x2000 && yim <0x2000);

	y[i].re = yre;
    y[i].im = yim;
  }

  for (i = (n4+1)/2; i < n4; i++) {
    re = (Ipp32s)(pSrc[2*i-n4]   ) - pSrc[n-n4-1-2*i];
    im = (Ipp32s)(-pSrc[n-1+n4-2*i]) - pSrc[n4+2*i];


	yre = (Ipp32s)(re * pSinCos[2*i+1] + im * pSinCos[2*i] );
	yim = (Ipp32s)(im * pSinCos[2*i+1] - re * pSinCos[2*i] );

	if(yre >=0)
		yre = (Ipp16s)((yre + round) >> (16 - scalef));
	else
	{
		yre = (Ipp16s)(((-yre - round) >> (16 - scalef)) * -1);
	}

	if(yim >=0)
		yim = (Ipp16s)((yim + round) >> (16 - scalef));
	else
	{
		yim = (Ipp16s)(((-yim - round) >> (16 - scalef)) * -1);
	}

	//yre = (Ipp32s)((re * pSinCos[2*i+1] + im * pSinCos[2*i] + round) >> (16 - scalef));
	//yim = (Ipp32s)((im * pSinCos[2*i+1] - re * pSinCos[2*i] + round) >> (16 - scalef));

    y[i].re = yre;
    y[i].im = yim;
  }
}

static void AAC_MDCTFwdPostProc_16s(Ipp16sc  *y,  Ipp32s   *pDst,  int      n, Ipp16s   *pSinCos)
{
  int     n2 = n / 2;
  int     n4 = n / 4;
  int     i;
  Ipp32s  re, im;


  for (i = 0; i < n4; i++) {
    re = (Ipp32s)y[i].re;
    im = (Ipp32s)y[i].im;
    pDst[2*i]      = re * pSinCos[2 * i + 1] + im * pSinCos[2 * i    ];
	//if( pDst[2*i] >= ERRO )
	//	 pDst[2*i] = ERRO-1;
    pDst[n2-1-2*i] = re * pSinCos[2 * i    ] - im * pSinCos[2 * i + 1];
	//if( pDst[n2-1-2*i] >= ERRO)
	//	 pDst[n2-1-2*i] = ERRO-1;
  }
}
IppStatus __STDCALL AAC_Convert_64s16s_Sfs(const Ipp64s* pSrc, Ipp16s* pDst, int len, int scaleFactor)
{
#if 1
	int i;
	int tmp;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			pDst[i] = (Ipp16s)CLIP_R(pSrc[i],MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		int sign;
		__int64 rounding = ((__int64)1<<(scaleFactor-1));
		int scale = (scaleFactor -1);
		__int64 times = (__int64)1<<scale;
		int shift = 64-scaleFactor;

		for (i = 0; i < len; i++)
		{
			tmp = pSrc[i];
			if (tmp != 0)
			{
				sign = 1;
				if (tmp < 0)
				{
					sign = -1; tmp = -tmp;
				}
				if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
					tmp = (tmp)>>scaleFactor;
				else
					tmp = (tmp+rounding)>>scaleFactor;
				pDst[i] = (Ipp16s)CLIP_R((sign * tmp),MAX_16S, MIN_16S);
			}
			else
			{
				pDst[i] = 0;
			}
		}
	}
	else if (scaleFactor < 0)
	{
		int scale = -scaleFactor;
		for (i = 0; i < len; i++)
		{
			tmp = pSrc[i];
			if ( (tmp > 0 && (MAX_16S>>scale) >= tmp)  || (tmp < 0 && (MIN_16S >> scale <= tmp)) )
				pDst[i] = (Ipp16s)(tmp << scale);
			else
				pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsConvert_32s16s_Sfs_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsConvert_32s16s_Sfs_c(pSrc, pDst,  len, scaleFactor);
		return sts;
	}
#endif
	return ippStsNoErr;
}




#endif

static void AAC_MDCTFwdPreProc_32s(Ipp16s  *pSrc,   Ipp32sc *y,     int     n,   int     scalef,    Ipp16s  *pSinCos)
{
  int     n4    = n / 4;
  Ipp64s  round = 0;
  int     i;
  Ipp64s  re, im, yre, yim;

  if (scalef < 16) {
    round = ((Ipp32s)1) << (15 - scalef);
  }

  for (i = 0; i < (n4+1)/2; i++) {
    re = (Ipp32s)((-pSrc[n-n4+2*i]) - pSrc[n-n4-1-2*i]);
    im = (Ipp32s)((pSrc[n4-1-2*i]) - pSrc[n4+2*i]);

	yre = (Ipp32s)(re * pSinCos[2*i+1] + im * pSinCos[2*i] );
	yim = (Ipp32s)(im * pSinCos[2*i+1] - re * pSinCos[2*i] );

	if(yre >=0)
		yre = (Ipp32s)((yre + round) >> (16 - scalef));
	else
	{
		yre = (Ipp32s)(((-yre - round) >> (16 - scalef)) * -1);
	}

	if(yim >=0)
		yim = (Ipp32s)((yim + round) >> (16 - scalef));
	else
	{
		yim = (Ipp32s)(((-yim - round) >> (16 - scalef)) * -1);
	}

	//yre = (Ipp32s)((re * pSinCos[2*i+1] + im * pSinCos[2*i] + round) >> (16 - scalef));
	//yim = (Ipp32s)((im * pSinCos[2*i+1] - re * pSinCos[2*i] + round) >> (16 - scalef));

	//assert(yre<0x2000 && yim <0x2000);

	y[i].re = yre;
    y[i].im = yim;
  }

  for (i = (n4+1)/2; i < n4; i++) {
    re = (Ipp32s)(pSrc[2*i-n4]   ) - pSrc[n-n4-1-2*i];
    im = (Ipp32s)(-pSrc[n-1+n4-2*i]) - pSrc[n4+2*i];


	yre = (Ipp32s)(re * pSinCos[2*i+1] + im * pSinCos[2*i] );
	yim = (Ipp32s)(im * pSinCos[2*i+1] - re * pSinCos[2*i] );

	if(yre >=0)
		yre = (Ipp32s)((yre + round) >> (16 - scalef));
	else
	{
		yre = (Ipp32s)(((-yre - round) >> (16 - scalef)) * -1);
	}

	if(yim >=0)
		yim = (Ipp32s)((yim + round) >> (16 - scalef));
	else
	{
		yim = (Ipp32s)(((-yim - round) >> (16 - scalef)) * -1);
	}

	//yre = (Ipp32s)((re * pSinCos[2*i+1] + im * pSinCos[2*i] + round) >> (16 - scalef));
	//yim = (Ipp32s)((im * pSinCos[2*i+1] - re * pSinCos[2*i] + round) >> (16 - scalef));

    y[i].re = yre;
    y[i].im = yim;
  }
}

// Post-process

#define ERRO 0x100000

static void AAC_MDCTFwdPostProc_32s(Ipp32sc  *y,  Ipp32s   *pDst,  int      n, Ipp16s   *pSinCos)
{
  int     n2 = n / 2;
  int     n4 = n / 4;
  int     i;
  Ipp32s  re, im;


  for (i = 0; i < n4; i++) {
    re = (Ipp32s)y[i].re;
    im = (Ipp32s)y[i].im;
    pDst[2*i]      = re * pSinCos[2 * i + 1] + im * pSinCos[2 * i    ];	// It is better Or safe to add clip operation here. (WWD)

    pDst[n2-1-2*i] = re * pSinCos[2 * i    ] - im * pSinCos[2 * i + 1];

  }
}
IppStatus __STDCALL ippsMDCTFwdGetBufSize_16s_c(const IppsMDCTFwdSpec_16s *pMDCTSpec, int *pSize)
{
	IppMDCTContext_16s *pkSpec = (IppMDCTContext_16s*)pMDCTSpec;

	if (!pkSpec)
		return ippStsNullPtrErr;


	*pSize = pkSpec->bufSize;

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMDCTFwdFree_16s_c(IppsMDCTFwdSpec_16s* pMDCTSpec)
{
  IppMDCTContext_16s  *pCtxMDCT;

  pCtxMDCT = (IppMDCTContext_16s*)pMDCTSpec;
  AAC_deleteMdctCtx(pCtxMDCT);
  return (ippStsNoErr);
}



IppStatus __STDCALL ippsMDCTFwdInitAlloc_16s_c(IppsMDCTFwdSpec_16s ** ppMDCTSpec, int len)
{
  IppStatus           res;
  IppMDCTContext_16s  *pCtxMDCT;

  res = AAC_MDCTinitAlloc_16s(&pCtxMDCT, len);
  AAC_FwdFillSinCosBuf_16s(pCtxMDCT->sincos, len);

  if (res == ippStsNoErr) {
    *ppMDCTSpec = (IppsMDCTFwdSpec_16s*)pCtxMDCT;
  }

  return (res);

}


/************************************************************************************************
 * This function will generate different result when compared with IPP functions
 *  Reason: 
 *
 *************************************************************************************************/
// pSrc may include 2048 elements
IppStatus __STDCALL ippsMDCTFwd_16s_Sfs_c(const Ipp16s *pSrc, Ipp16s *pDst,const IppsMDCTFwdSpec_16s* pMDCTSpec,int scaleFactor, Ipp8u* pBuffer)
{
  IppMDCTContext_16s *pCtxMDCT  = (IppMDCTContext_16s*)pMDCTSpec;
  IppStatus          res        = ippStsNoErr;
#ifndef _32BIT_FFT_
  Ipp16sc            *pBuf      = 0;
#else
  Ipp32sc            *pBuf      = 0;
#endif
  Ipp16s             min0, max0;
  int                len, min, max, scalef;

#ifndef _32BIT_FFT_
  if (pBuffer != 0) {
    pBuf = (Ipp16sc*)ALIGNED_PTR(pBuffer, MALLOC_ALIGNED_BYTES);
  } else {
    pBuf = (Ipp16sc*)ippsMalloc_8u_c(pCtxMDCT->bufSize);
    if (pBuf == 0) {
      return ippStsMemAllocErr;
    }
  }
#else
 if (pBuffer != 0) {
    pBuf = (Ipp32sc*)ALIGNED_PTR(pBuffer, MALLOC_ALIGNED_BYTES);
  } else {
    pBuf = (Ipp32sc*)ippsMalloc_8u_c(pCtxMDCT->bufSize * 2);
    if (pBuf == 0) {
      return ippStsMemAllocErr;
    }
  }
#endif

  len = pCtxMDCT->len;

  ippsMinMax_16s_c(pSrc, len, &min0, &max0);

  min = -((int)min0);
  max = max0;

  if (min > max) {
    max = min;
  }

  if (max == 0) {
    ippsZero_16s_c(pDst, len/2);
    GOTO_RET(ippStsNoErr);
  }

  scalef = 0;

  while (max <= 0x3FFF) {  // Not sure about this value
    max *= 2;
    scalef++;
  }

  if (IS_POWER_2(len)) 
  {
	// AAC_MDCTFwdPreProc_16s has changed input data from 2048(real) -- >512(complex)
#ifndef _32BIT_FFT_
    AAC_MDCTFwdPreProc_16s((Ipp16s*)pSrc, pBuf, len, scalef, pCtxMDCT->sincos);
    res = ippsFFTFwd_CToC_16sc_Sfs_c(pBuf, pBuf, pCtxMDCT->ctxFFT, pCtxMDCT->order + 1, (Ipp8u*)(pBuf+len/4));
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }

    AAC_MDCTFwdPostProc_16s(pBuf, (Ipp32s*)(pBuf+len/4), len, pCtxMDCT->sincos);
    ippsConvert_32s16s_Sfs_c((Ipp32s*)(pBuf+len/4), pDst, len/2, scaleFactor - pCtxMDCT->order + 10 + scalef);  // Here is 10, because we do NOT do /2 in AAC_MDCTFwdPostProc_16s 
#else
	AAC_MDCTFwdPreProc_32s((Ipp16s*)pSrc, pBuf, len, scalef, pCtxMDCT->sincos);
    res = ippsFFTFwd_CToC_32sc_Sfs_c(pBuf, pBuf, pCtxMDCT->ctxFFT, pCtxMDCT->order + 1 /*0?? */, (Ipp8u*)(pBuf+len/4));
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }

    AAC_MDCTFwdPostProc_32s(pBuf, (Ipp32s*)(pBuf+len/4), len, pCtxMDCT->sincos);
    ippsConvert_32s16s_Sfs_c((Ipp32s*)(pBuf+len/4), pDst, len/2, scaleFactor - pCtxMDCT->order + 10 + scalef);  // Here is 10, because we do NOT do /2 in AAC_MDCTFwdPostProc_16s 
#endif
  }

RET:
  if ((pBuffer == 0) && (pBuf != 0)) {
    ippsFree_c(pBuf);
  }
  return (res);
}


#ifdef KEEP_UNUSED_KINOMA_IPP_IMPLEMENTATION	//****not needed yet

IppStatus __STDCALL ippsFFTInitAlloc_C_16s_c( IppsFFTSpec_C_16s** ppFFTSpec,  int order, int flag, IppHintAlgorithm hint )
{
	k_IppsFFTSpec_C_16s *pkSpec;
	int buffSize = (1<<(order+2)) * 2 + 24;
	
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
	}
	else if (flag == IPP_FFT_DIV_FWD_BY_N)
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	*ppFFTSpec = (IppsFFTSpec_C_16s*)pkSpec;

	return ippStsNoErr;
}
IppStatus __STDCALL ippsFFTFree_C_16s_c ( IppsFFTSpec_C_16s*  pFFTSpec )
{
	k_IppsFFTSpec_C_16s *pkSpec = (k_IppsFFTSpec_C_16s*)pFFTSpec;
	
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

IppStatus __STDCALL ippsFFTGetBufSize_R_32s_c (const IppsFFTSpec_R_32s*  pFFTSpec, int* pSize )
{
	k_IppsFFTSpec_R_32s *pkSpec = (k_IppsFFTSpec_R_32s*)pFFTSpec;

	if (!pFFTSpec)
		return ippStsNullPtrErr;


	*pSize = pkSpec->sizeWorkBuf + 16;

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTFwd_RToPack_32s_Sfs_c(const Ipp32s* pSrc, Ipp32s* pDst,   const IppsFFTSpec_R_32s* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )
{
	k_IppsFFTSpec_R_32s * pkSpec = (k_IppsFFTSpec_R_32s*)pFFTSpec;

	int i,j,k;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	double *xr, *xi, tmp;

	int  len = pkSpec->len/4;


	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	if (pkSpec->sig != COMPLEX_SIG)		
		return ippStsContextMatchErr;	

	xr = pkSpec->pbuff;
	xi = (pkSpec->pbuff + len);

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

	if (pkSpec->flagFwd)
	{
		int c;

		if (xr[i] < 0)
			pDst[0] = (Ipp32s)(((__int64)((xr[0]/pkSpec->normFwd) - rounding)) >> scaleFactor);
		else
			pDst[0] = (Ipp32s)(((__int64)((xr[0]/pkSpec->normFwd) + rounding)) >> scaleFactor);

		c = 1;
		for (i = 1; i < len/2; i++)
		{
			if (xr[i] < 0)
				pDst[c++] = (Ipp32s)(((__int64)((xr[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp32s)(((__int64)((xr[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[c++] = (Ipp32s)(((__int64)((xi[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp32s)(((__int64)((xi[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);
		}

		if (xr[i] < 0)
			pDst[c++] = (Ipp32s)(((__int64)((xr[len/2]/pkSpec->normFwd) - rounding)) >> scaleFactor);
		else
			pDst[c++] = (Ipp32s)(((__int64)((xr[len/2]/pkSpec->normFwd) + rounding)) >> scaleFactor);

	}
	else
	{
		int c;
		if (xr[i] < 0)
			pDst[0] = (Ipp16s)(((__int64)(xr[0] - rounding)) >> scaleFactor);
		else
			pDst[0] = (Ipp16s)(((__int64)(xr[0] + rounding)) >> scaleFactor);

		c = 1;
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

	}
	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTInitAlloc_R_32s_c(IppsFFTSpec_R_32s** ppFFTSpec,int order, int flag, IppHintAlgorithm hint )
{
	k_IppsFFTSpec_R_32s *pkSpec;
	int buffSize = (1<<(order+2)) * 4 + 32;
	int i, halfsize, sampleSize;	
	double theta;
	double ang;
	int len;
	
	if (!ppFFTSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;
	
	pkSpec = (k_IppsFFTSpec_R_32s*)ippsMalloc_8u_c(sizeof(k_IppsFFTSpec_R_32s));
	if (!pkSpec)
		return ippStsMemAllocErr;

	pkSpec->sig = 0x4c4d5365;							/*means Real fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = buffSize;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 1;									/*means this is InitAlloc func, not Init*/
	pkSpec->startAddr = 0;
	pkSpec->dummy0 = pkSpec->dummy1 = 0xbaadf00d;

	// 
	pkSpec->len = 1 << (order+2);							/* 2^order will be the input sample length */
	len = pkSpec->len/4;


	/*sin and cos table*/
	halfsize = pkSpec->len >> 1;
	pkSpec->cos = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);
	pkSpec->sin = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);
	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)pkSpec->len;
		pkSpec->cos[i] = cos(theta);
		pkSpec->sin[i] = -sin(theta);
	}

	/*bit reverse table*/
	pkSpec->ReverseTbl = (int*)ippsMalloc_8u_c(sizeof(int)*pkSpec->len);
	for (i = 0; i < len; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < pkSpec->order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		pkSpec->ReverseTbl[i] = reversed;
	}	


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

	pkSpec->pbuff  = (double*)ippsMalloc_8u_c(sizeof(double)*len*8);  // Workaroud

	*ppFFTSpec = (IppsFFTSpec_R_32s*)pkSpec;


	return ippStsNoErr;
}

// We need one InvFFT
//IppStatus __STDCALL ippsFFTInv_CToC_32sc_Sfs_c(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)
IppStatus __STDCALL ippsFFTInv_PackToR_32s_Sfs_c(const Ipp32s* pSrc, Ipp32s* pDst, const IppsFFTSpec_R_32s* pFFTSpec,   int scaleFactor, Ipp8u* pBuffer )
{
	k_IppsFFTSpec_C_32sc * pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	int i,j;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	double *xr, *xi, tmp;
	int  len = pkSpec->len/4;
	int	 c=0;


	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	if (pkSpec->sig != COMPLEX_SIG)		
		return ippStsContextMatchErr;	

	xr = (double*)pBuffer;
	xi = (double*)(pBuffer + pkSpec->len * sizeof(double));

	// First do unpack
	xr[0] = pSrc[c++]; 
	xi[0] = 0;
	for(i=1; i< len/2; i++)
	{
		xr[i] = pSrc[c++]; 
		xi[i] = pSrc[c++];
	}
	
	//if (len%2==0)
	{
		xr[len/2] = pSrc[c];
		xi[len/2] = 0;

		for (i = i+1; i < len; i++)
		{
			xi[i] = -pSrc[--c];
			xr[i] = pSrc[--c];
		}
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

	/*Inv transform*/
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
	for (i = 0; i < pkSpec->len; i++)
	{
		xi[i] = -xi[i];
	}
	if (scaleFactor)
		rounding = (1<< (scaleFactor -1 ))+0.5;
	else
		rounding = 0.5;
	if (pkSpec->flagInv)
	{
		int c=0;
		for (i = 0; i < len; i++)
		{
			if (xr[i] < 0)
				pDst[c++] = (Ipp32s)(((__int64)((xr[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp32s)(((__int64)((xr[i]/pkSpec->normInv) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[c++] = (Ipp32s)(((__int64)((xi[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp32s)(((__int64)((xi[i]/pkSpec->normInv) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		int c=0;
		for (i = 0; i < len; i++)
		{
			if (xr[i] < 0)
				pDst[c++] = (Ipp32s)(((__int64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp32s)(((__int64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[c++] = (Ipp32s)(((__int64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[c++] = (Ipp32s)(((__int64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
	return ippStsNoErr;
}
#endif


#if 0
// First make this function as inter function --- WWD in 20070422 --- in order to increase presicion
IppStatus __STDCALL AAC_FFTFwd_CToC_32sc_Sfs (const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_16sc* pFFTSpec,  int scaleFactor, Ipp8u* pBuffer )
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
				pDst[i].re = (Ipp32s)(((__int64)((xr[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((__int64)((xr[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((__int64)((xi[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((__int64)((xi[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((__int64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((__int64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((__int64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((__int64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
	return ippStsNoErr;
}


#endif
#endif
