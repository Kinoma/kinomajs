/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives AAC Decode Sample for Windows*
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//  MPEG-4 and AAC are international standards promoted by ISO, IEC, ITU, ETSI
//    and other organizations. Implementations of these standards, or the standard
//    enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
*/

/********************************************************************/
//***
#include "kinoma_ipp_lib.h"

#include <stdlib.h>
#include <math.h>
#include "aac_filterbank_int.h"
#include "aac_wnd_tables_int.h"

/********************************************************************/

#undef IS_POWER_2
#undef GOTO_RET
#undef RESERVE_SIZE
#undef MDCT_BUFSIZE_COUNT

/*******************************************************************/

#define IS_POWER_2(len)  (((len-1)&len)==0)
#define GOTO_RET(stat)   { res = stat; goto RET; }

/*******************************************************************/

#define MALLOC_ALIGNED_BYTES 32
#define ALIGNED_PTR(ptr, bytes) \
  (ptr + ((bytes - ((Ipp32s)ptr & (bytes-1))) & (bytes-1)))
#define RESERVE_SIZE     MALLOC_ALIGNED_BYTES

#define RETURN_ALIGNED_MEM(ptr) ALIGNED_PTR(ptr, MALLOC_ALIGNED_BYTES)

/*******************************************************************/

#define MDCT_BUFSIZE_COUNT(FFTBufSize, MDCTBufSize)                   \
  MDCTBufSize = FFTBufSize;                                           \
  if (MDCTBufSize < ((len)*(int)sizeof(Ipp32sc)))                     \
      MDCTBufSize = ((len)*(int)sizeof(Ipp32sc));                     \
                                                                      \
  MDCTBufSize += ((len/4)*sizeof(Ipp32sc) + MALLOC_ALIGNED_BYTES);

/*******************************************************************/

typedef struct {
  int     len;
  int     bufSize;
  int     order;
  int     alloc;
  Ipp32s *sincos;
  IppsFFTSpec_C_32sc *ctxFFT;
} IppMDCTContext_32s;

/*******************************************************************/

static void deleteMdctCtx(IppMDCTContext_32s * pCtxMDCT)
{
  if (pCtxMDCT != NULL) {
//    pCtxMDCT->idCtx = idCtxUnknown;
    if (pCtxMDCT->alloc != 0) {
      if (pCtxMDCT->ctxFFT != NULL) {
        ippsFFTFree_C_32sc_x(pCtxMDCT->ctxFFT);
      }
      ippsFree_x(pCtxMDCT);
    }
  }
}

/*******************************************************************/

static void ownMDCTFwdPreProc_32s(Ipp32s  *pSrc,
                                  Ipp32sc *y,
                                  int     n,
                                  int     scalef,
                                  Ipp32s  *pSinCos)
{
  int     n4    = n / 4;
  Ipp64s  round = 0;
  int     i;
  Ipp64s  re, im;

  if (scalef < 32) {
    round = ((Ipp64s)1) << (31 - scalef);
  }

  for (i = 0; i < (n4+1)/2; i++) {
    re = (Ipp64s)(-pSrc[n-n4+2*i]) - pSrc[n-n4-1-2*i];
    im = (Ipp64s)(pSrc[n4-1-2*i]) - pSrc[n4+2*i];
    y[i].re = (Ipp32s)((re * pSinCos[2*i+1] + im * pSinCos[2*i] + round) >> (32 - scalef));
    y[i].im = (Ipp32s)((im * pSinCos[2*i+1] - re * pSinCos[2*i] + round) >> (32 - scalef));
  }

  for (i = (n4+1)/2; i < n4; i++) {
    re = (Ipp64s)(pSrc[2*i-n4]   ) - pSrc[n-n4-1-2*i];
    im = (Ipp64s)(-pSrc[n-1+n4-2*i]) - pSrc[n4+2*i];
    y[i].re = (Ipp32s)((re * pSinCos[2*i+1] + im * pSinCos[2*i] + round) >> (32 - scalef));
    y[i].im = (Ipp32s)((im * pSinCos[2*i+1] - re * pSinCos[2*i] + round) >> (32 - scalef));
  }
}

/*******************************************************************/

static void ownMDCTInvPreProc_32s(Ipp32s  *pSrc,
                                  Ipp32sc *y,
                                  int     n,
                                  int     scalef,
                                  Ipp32s  *pSinCos)
{
  int     n2    = n / 2;
  int     n4    = n / 4;
  Ipp64s  round = 0;
  int     i;
  Ipp64s  re, im;

  if (scalef < 32) {
    round = ((Ipp64s)1) << (31 - scalef);
  }

  for (i = 0; i < n4; i++) {
    re = (Ipp64s)pSrc[2 * i];
    im = (Ipp64s)pSrc[n2 - 1 - 2 * i];
    y[i].re = (Ipp32s)((re * pSinCos[2 * i + 1] + im * pSinCos[2 * i] + round) >> (32 - scalef));
    y[i].im = (Ipp32s)((im * pSinCos[2 * i + 1] - re * pSinCos[2 * i] + round) >> (32 - scalef));
  }
}

/*******************************************************************/

static void ownMDCTFwdPostProc_32s(Ipp32sc  *y,
                                   Ipp64s   *pDst,
                                   int      n,
                                   Ipp32s   *pSinCos)
{
  int     n2 = n / 2;
  int     n4 = n / 4;
  int     i;
  Ipp64s  re, im;

  for (i = 0; i < n4; i++) {
    re = (Ipp64s)y[i].re;
    im = (Ipp64s)y[i].im;
    pDst[2*i]      = re * pSinCos[2 * i + 1] + im * pSinCos[2 * i    ];
    pDst[n2-1-2*i] = re * pSinCos[2 * i    ] - im * pSinCos[2 * i + 1];
  }
}

/*******************************************************************/

static void ownMDCTInvPostProc_32s(Ipp32sc  *y,
                                   Ipp64s   *pDst,
                                   int      n,
                                   Ipp32s   *pSinCos)
{
  int     n4 = n / 4;
  int     i;
  Ipp64s  re, im;

  for (i = 0; i < (n4 + 1) / 2; i++) {
    re = (Ipp64s)y[i].re * pSinCos[2 * i + 1] + (Ipp64s)y[i].im * pSinCos[2 * i];
    im = (Ipp64s)y[i].im * pSinCos[2 * i + 1] - (Ipp64s)y[i].re * pSinCos[2 * i];
    pDst[n - n4 + 2 * i] = -re;
    pDst[n - n4 - 1 - 2 * i] = -re;
    pDst[n4 + 2 * i] = im;
    pDst[n4 - 1 - 2 * i] = -im;
  }
  for (i = (n4 + 1) / 2; i < n4; i++) {
    re = (Ipp64s)y[i].re * pSinCos[2 * i + 1] + (Ipp64s)y[i].im * pSinCos[2 * i];
    im = (Ipp64s)y[i].im * pSinCos[2 * i + 1] - (Ipp64s)y[i].re * pSinCos[2 * i];
    pDst[2 * i - n4] = re;
    pDst[n - n4 - 1 - 2 * i] = -re;
    pDst[n4 + 2 * i] = im;
    pDst[n - 1 + n4 - 2 * i] = im;
  }
}

/*******************************************************************/

static void ownFwdFillSinCosBuf_32s(Ipp32s *sincos,
                                    Ipp32s len)
{
  double ang;
  int    i;

  ang = IPP_2PI/len;
  for (i = 0; i < len/4; i++) {
    sincos[2*i  ] = (Ipp32s)(sin(ang * (i+0.125)) * 1073741824 + 0.5);    /* 1073741824 = 2^30 */
    sincos[2*i+1] = (Ipp32s)(cos(ang * (i+0.125)) * 1073741824 + 0.5);
  }
}

/*******************************************************************/

static IppStatus ownMDCTinitAlloc(IppMDCTContext_32s  **ppCtxMDCT,
                                  int                 len)
{
  IppMDCTContext_32s *pCtxMDCT = NULL;
  IppStatus          res;
  Ipp32s             *sincos;
  int                i, order = 0;
  int                stateSize, bufSize = 0, FFTBufSize = 0;

  stateSize = sizeof(IppMDCTContext_32s) + (len/2) * sizeof(Ipp32s) + MALLOC_ALIGNED_BYTES;
  pCtxMDCT = (IppMDCTContext_32s*)ippsMalloc_8u_x(stateSize);
  if (pCtxMDCT == NULL) {
    return ippStsMemAllocErr;
  }
  ippsZero_8u_x((Ipp8u*)pCtxMDCT, sizeof(IppMDCTContext_32s));

  pCtxMDCT->alloc = 1;

  sincos = (Ipp32s*)((Ipp8u*)pCtxMDCT + sizeof(IppMDCTContext_32s));
  sincos = (Ipp32s*)ALIGNED_PTR(sincos, MALLOC_ALIGNED_BYTES);
  pCtxMDCT->sincos = sincos;
  pCtxMDCT->len = len;

  if (IS_POWER_2(len)) {
    for (i = 1; i < len/4; i *= 2) {
      order++;
    }
    res = ippsFFTInitAlloc_C_32sc_x(&pCtxMDCT->ctxFFT, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone);
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }
    res = ippsFFTGetBufSize_C_32sc_x(pCtxMDCT->ctxFFT, &FFTBufSize);
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }
  }

  MDCT_BUFSIZE_COUNT(FFTBufSize, bufSize)
  pCtxMDCT->bufSize = bufSize;
  pCtxMDCT->order = order;

  *ppCtxMDCT = pCtxMDCT;
  return (ippStsNoErr);

RET:
  deleteMdctCtx(pCtxMDCT);
  return (res);
}

/*******************************************************************/
#if 0
static IppStatus ownMDCTinit(IppMDCTContext_32s **ppCtxMDCT,
                             int                len,
                             Ipp8u              *pMemSpec,
                             Ipp8u              *pMemInit)
{
  IppMDCTContext_32s *pCtxMDCT = NULL;
  IppStatus          res;
  Ipp32s             *sincos;
  int                i, order = 0;
  int                bufSize, FFTBufSize = 0;

  pCtxMDCT = (IppMDCTContext_32s*)RETURN_ALIGNED_MEM(pMemSpec);
  ippsZero_8u_x((Ipp8u*)pCtxMDCT, sizeof(IppMDCTContext_32s));

  pCtxMDCT->alloc = 0;

  sincos = (Ipp32s*)((Ipp8u*)pCtxMDCT + sizeof(IppMDCTContext_32s));
  sincos = (Ipp32s*)ALIGNED_PTR(sincos, MALLOC_ALIGNED_BYTES);
  pCtxMDCT->sincos = sincos;

  pCtxMDCT->len = len;
  pCtxMDCT->ctxFFT = NULL;

  if (IS_POWER_2(len)) {
    for (i = 1; i < len/4; i *= 2) {
      order++;
    }
    res = ippsFFTInit_C_32sc_x(&pCtxMDCT->ctxFFT, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, (Ipp8u*)(sincos + (len/2)), pMemInit);
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }
    res = ippsFFTGetBufSize_C_32sc_x(pCtxMDCT->ctxFFT, &FFTBufSize);
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }
  }

  MDCT_BUFSIZE_COUNT(FFTBufSize, bufSize)
  pCtxMDCT->bufSize = bufSize;
  pCtxMDCT->order = order;

  *ppCtxMDCT = pCtxMDCT;
  return (ippStsNoErr);

RET:
  deleteMdctCtx(pCtxMDCT);
  return (res);
}
#endif
/*******************************************************************/

static IppStatus ippsMDCTFwdInitAlloc_32s(IppsMDCTFwdSpec_32s  **ppMDCTSpec,
                                          int                  len)
{
  IppStatus           res;
  IppMDCTContext_32s  *pCtxMDCT;

  res = ownMDCTinitAlloc(&pCtxMDCT, len);
  ownFwdFillSinCosBuf_32s(pCtxMDCT->sincos, len);

  if (res == ippStsNoErr) {
    *ppMDCTSpec = (IppsMDCTFwdSpec_32s*)pCtxMDCT;
  }

  return (res);
}

/*******************************************************************/

static IppStatus ippsMDCTInvInitAlloc_32s(IppsMDCTInvSpec_32s  **ppMDCTSpec,
                                          int                  len)
{
  IppStatus           res;
  IppMDCTContext_32s  *pCtxMDCT;

  res = ownMDCTinitAlloc(&pCtxMDCT, len);
  ownFwdFillSinCosBuf_32s(pCtxMDCT->sincos, len);

  if (res == ippStsNoErr) {
    *ppMDCTSpec = (IppsMDCTInvSpec_32s*)pCtxMDCT;
  }

  return (res);
}

/*******************************************************************/
#if 0
static IppStatus ippsMDCTFwdInit_32s(IppsMDCTFwdSpec_32s **ppMDCTSpec,
                                     int                 len,
                                     Ipp8u               *pMemSpec,
                                     Ipp8u               *pMemInit)
{
  IppStatus           res;
  IppMDCTContext_32s  *pCtxMDCT;

  res = ownMDCTinit(&pCtxMDCT, len, pMemSpec, pMemInit);
  ownFwdFillSinCosBuf_32s(pCtxMDCT->sincos, len);

  if (res == ippStsNoErr) {
    *ppMDCTSpec = (IppsMDCTFwdSpec_32s*)pCtxMDCT;
  }
  return (res);
}

/*******************************************************************/

static IppStatus ippsMDCTInvInit_32s (IppsMDCTInvSpec_32s **ppMDCTSpec,
                                      int                 len,
                                      Ipp8u               *pMemSpec,
                                      Ipp8u               *pMemInit)
{
  IppStatus           res;
  IppMDCTContext_32s  *pCtxMDCT;

  res = ownMDCTinit(&pCtxMDCT, len, pMemSpec, pMemInit);
  ownFwdFillSinCosBuf_32s(pCtxMDCT->sincos, len);

  if (res == ippStsNoErr) {
    *ppMDCTSpec = (IppsMDCTInvSpec_32s*)pCtxMDCT;
  }
  return (res);
}
#endif
/*******************************************************************/

static IppStatus ippsMDCTFwdFree_32s (IppsMDCTFwdSpec_32s *pMDCTSpec)
{
  IppMDCTContext_32s  *pCtxMDCT;

  pCtxMDCT = (IppMDCTContext_32s*)pMDCTSpec;
  deleteMdctCtx(pCtxMDCT);
  return (ippStsNoErr);
}

/*******************************************************************/

static IppStatus ippsMDCTInvFree_32s (IppsMDCTInvSpec_32s *pMDCTSpec)
{
  IppMDCTContext_32s  *pCtxMDCT;

  pCtxMDCT = (IppMDCTContext_32s*)pMDCTSpec;
  deleteMdctCtx(pCtxMDCT);
  return (ippStsNoErr);
}

/*******************************************************************/
#if 0
static IppStatus ippsMDCTFwdGetSize_32s (int len,
                                         int *pSizeSpec,
                                         int *pSizeInit,
                                         int *pSizeBuf)
{
  IppStatus res;
  int       i;
  int       order = 0;
  int       FFTSpecSize = 0, bufSize = 0, MDCTSpecSize;

  *pSizeInit = 0;

  if (IS_POWER_2(len)) {
    for (i = 1; i < len/4; i *= 2) {
      order++;
    }

    res = ippsFFTGetSize_C_32sc_x(order, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, &FFTSpecSize, pSizeInit, &bufSize);
    if (res != ippStsNoErr) {
      return (res);
    }
  }

  MDCT_BUFSIZE_COUNT(bufSize, bufSize)
  *pSizeBuf = bufSize;
  MDCTSpecSize = sizeof(IppMDCTContext_32s) + (len/2) * sizeof(Ipp32s) + 2 * MALLOC_ALIGNED_BYTES;

  *pSizeSpec = FFTSpecSize + MALLOC_ALIGNED_BYTES + MDCTSpecSize;

  return (ippStsNoErr);
}

/*******************************************************************/

static IppStatus ippsMDCTInvGetSize_32s (int len,
                                         int *pSizeSpec,
                                         int *pSizeInit,
                                         int *pSizeBuf)
{
  IppStatus res;
  int       i;
  int       order = 0;
  int       FFTSpecSize = 0, bufSize = 0, MDCTSpecSize;

  *pSizeInit = 0;

  if (IS_POWER_2(len)) {
    for (i = 1; i < len/4; i *= 2) {
      order++;
    }

    res = ippsFFTGetSize_C_32sc_x(order, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, &FFTSpecSize, pSizeInit, &bufSize);
    if (res != ippStsNoErr) {
      return (res);
    }
  }

  MDCT_BUFSIZE_COUNT(bufSize, bufSize)
  *pSizeBuf = bufSize;
  MDCTSpecSize = sizeof(IppMDCTContext_32s) + (len/2) * sizeof(Ipp32s) + 2 * MALLOC_ALIGNED_BYTES;

  *pSizeSpec = FFTSpecSize + MALLOC_ALIGNED_BYTES + MDCTSpecSize;

  return (ippStsNoErr);
}
#endif
/*******************************************************************/

static IppStatus ippsMDCTFwdGetBufSize_32s (const IppsMDCTFwdSpec_32s *pMDCTSpec,
                                            int                       *pSize)
{
  IppMDCTContext_32s *pCtxMDCT = (IppMDCTContext_32s*)pMDCTSpec;

  *pSize = pCtxMDCT->bufSize;
  return (ippStsNoErr);
}


/*******************************************************************/

static IppStatus ippsMDCTInvGetBufSize_32s (const IppsMDCTInvSpec_32s *pMDCTSpec,
                                            int                 *pSize)
{
  IppMDCTContext_32s *pCtxMDCT = (IppMDCTContext_32s*)pMDCTSpec;
  *pSize = pCtxMDCT->bufSize;
  return (ippStsNoErr);
}


/*******************************************************************/

static IppStatus ippsMDCTFwd_32s_Sfs (const Ipp32s              *pSrc,
                                            Ipp32s              *pDst,
                                      const IppsMDCTFwdSpec_32s *pMDCTSpec,
                                            int                 scaleFactor,
                                            Ipp8u               *pBuffer)
{
  IppMDCTContext_32s *pCtxMDCT  = (IppMDCTContext_32s*)pMDCTSpec;
  IppStatus          res        = ippStsNoErr;
  Ipp32sc            *pBuf      = NULL;
  Ipp32s             min0, max0;
  int                len, min, max, scalef;

  if (pBuffer != NULL) {
    pBuf = (Ipp32sc*)ALIGNED_PTR(pBuffer, MALLOC_ALIGNED_BYTES);
  } else {
    pBuf = (Ipp32sc*)ippsMalloc_8u_x(pCtxMDCT->bufSize);
    if (pBuf == NULL) {
      return ippStsMemAllocErr;
    }
  }

  len = pCtxMDCT->len;

  ippsMinMax_32s_x(pSrc, len, &min0, &max0);

  min = -((int)min0);
  max = max0;

  if (min > max) {
    max = min;
  }

  if (max == 0) {
    ippsZero_32s_x(pDst, len/2);
    GOTO_RET(ippStsNoErr);
  }

  scalef = 0;

  while (max <= 0x3FFFFFFF) {
    max *= 2;
    scalef++;
  }

/*
  if (max <= 0x5a827999) {   * 0x80000000 / sgrt(2) *
    scalef++;
  }
*/
  if (IS_POWER_2(len)) {

    ownMDCTFwdPreProc_32s((Ipp32s*)pSrc, pBuf, len, scalef, pCtxMDCT->sincos);
    res = ippsFFTFwd_CToC_32sc_Sfs_x(pBuf, pBuf, pCtxMDCT->ctxFFT, pCtxMDCT->order + 1, (Ipp8u*)(pBuf+len/4));
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }

    ownMDCTFwdPostProc_32s(pBuf, (Ipp64s*)(pBuf+len/4), len, pCtxMDCT->sincos);
    ippsConvert_64s32s_Sfs_x((Ipp64s*)(pBuf+len/4), pDst, len/2, ippRndZero, scaleFactor - pCtxMDCT->order + 26 + scalef);

  }

RET:
  if (pBuffer == NULL) {
    ippsFree_x(pBuf);
  }
  return (res);
}

/*******************************************************************/

static IppStatus ippsMDCTInv_32s_Sfs (const Ipp32s              *pSrc,
                                            Ipp32s              *pDst,
                                      const IppsMDCTInvSpec_32s *pMDCTSpec,
                                            int                 scaleFactor,
                                            Ipp8u               *pBuffer)
{
  IppMDCTContext_32s *pCtxMDCT  = (IppMDCTContext_32s*)pMDCTSpec;
  IppStatus          res        = ippStsNoErr;
  Ipp32sc            *pBuf      = NULL;
  Ipp32s             min0, max0;
  int                len, min, max, scalef;

  if (pBuffer != NULL) {
    pBuf = (Ipp32sc*)ALIGNED_PTR(pBuffer, MALLOC_ALIGNED_BYTES);
  } else {
    pBuf = (Ipp32sc*)ippsMalloc_8u_x(pCtxMDCT->bufSize);
    if (pBuf == NULL) {
      return ippStsMemAllocErr;
    }
  }

  len = pCtxMDCT->len;

  ippsMinMax_32s_x(pSrc, len, &min0, &max0);

  min = -((int)min0);
  max = max0;

  if (min > max) {
    max = min;
  }

  if (max == 0) {
    ippsZero_32s_x(pDst, len);
    GOTO_RET(ippStsNoErr);
  }

  scalef = 0;

  while (max <= 0x3FFFFFFF) {
    max *= 2;
    scalef++;
  }

  if (IS_POWER_2(len)) {

    ownMDCTInvPreProc_32s((Ipp32s*)pSrc, pBuf, len, scalef, pCtxMDCT->sincos);

    res = ippsFFTFwd_CToC_32sc_Sfs_x(pBuf, pBuf, pCtxMDCT->ctxFFT, pCtxMDCT->order, (Ipp8u*)(pBuf+len/4));
    if (res != ippStsNoErr) {
      GOTO_RET(res);
    }

    ownMDCTInvPostProc_32s(pBuf, (Ipp64s*)(pBuf+len/4), len, pCtxMDCT->sincos);
    ippsConvert_64s32s_Sfs_x((Ipp64s*)(pBuf+len/4), pDst, len, ippRndZero, scaleFactor + 29 + scalef);

  }

RET:
  if (pBuffer == NULL) {
    ippsFree_x(pBuf);
  }
  return (res);
}

/*******************************************************************/

AACStatus InitFilterbankInt(sFilterbankInt* pBlock,
                            int mode)
{
  int size, size_short, size_long;

  ippsZero_8u_x((Ipp8u*)pBlock, sizeof(sFilterbankInt));

  if (pBlock == NULL)
    return AAC_ALLOC;

  size = 0;

  if (mode & FB_DECODER) {
    if (ippsMDCTInvInitAlloc_32s(&pBlock->p_mdct_inv_long, N_LONG) != ippStsOk)
      return AAC_ALLOC;

    if (ippsMDCTInvInitAlloc_32s(&pBlock->p_mdct_inv_short, N_SHORT) != ippStsOk)
      return AAC_ALLOC;

    if (ippsMDCTInvGetBufSize_32s(pBlock->p_mdct_inv_long, &size) != ippStsOk)
      return AAC_ALLOC;

    if (ippsMDCTInvGetBufSize_32s(pBlock->p_mdct_inv_short, &size_short) != ippStsOk)
      return AAC_ALLOC;

    if (size < size_short) size = size_short;
  }

  if (mode & FB_ENCODER) {
    if (ippsMDCTFwdInitAlloc_32s(&pBlock->p_mdct_fwd_long, N_LONG) != ippStsOk)
      return AAC_ALLOC;

    if (ippsMDCTFwdInitAlloc_32s(&pBlock->p_mdct_fwd_short, N_SHORT) != ippStsOk)
      return AAC_ALLOC;

    if (ippsMDCTFwdGetBufSize_32s(pBlock->p_mdct_fwd_long, &size_long) != ippStsOk)
      return AAC_ALLOC;

    if (ippsMDCTFwdGetBufSize_32s(pBlock->p_mdct_fwd_short, &size_short) != ippStsOk)
      return AAC_ALLOC;

    if (size < size_long) size = size_long;
    if (size < size_short) size = size_short;
  }

  if (size != 0) {
    pBlock->p_buffer_inv = ippsMalloc_8u_x(size);
    pBlock->p_buffer_fwd = pBlock->p_buffer_inv;
  }

  return AAC_OK;
}

/********************************************************************/

void FreeFilterbankInt(sFilterbankInt* pBlock)
{
  if (pBlock->p_mdct_inv_long)
      ippsMDCTInvFree_32s(pBlock->p_mdct_inv_long);
  if (pBlock->p_mdct_inv_short)
      ippsMDCTInvFree_32s(pBlock->p_mdct_inv_short);

  if (pBlock->p_mdct_fwd_long)
      ippsMDCTFwdFree_32s(pBlock->p_mdct_fwd_long);
  if (pBlock->p_mdct_fwd_short)
      ippsMDCTFwdFree_32s(pBlock->p_mdct_fwd_short);

  if (pBlock->p_buffer_inv)
    ippsFree_x(pBlock->p_buffer_inv);
}

/********************************************************************/

void FilterbankDecInt(sFilterbankInt* p_data,
                      Ipp32s* p_in_spectrum,
                      Ipp32s* p_in_prev_samples,
                      int window_sequence,
                      int window_shape,
                      int prev_window_shape,
                      Ipp32s* p_out_samples_1st_part,
                      Ipp32s* p_out_samples_2nd_part)
{
  Ipp32s samples[N_LONG];
  Ipp32s* p_samples;
  Ipp32s* p_spectrum;
  Ipp32s *window_table_long, *prev_window_table_long;
  Ipp32s *window_table_short, *prev_window_table_short;
  Ipp32s mdct_out_short[N_SHORT];
  int j;

  if (0 == window_shape) {
    window_table_long  = SIN_longInt;
    window_table_short = SIN_shortInt;
  } else {
    window_table_long  = KBD_longInt;
    window_table_short = KBD_shortInt;
  }

  if (0 == prev_window_shape) {
    prev_window_table_long  = SIN_longInt;
    prev_window_table_short = SIN_shortInt;
  } else {
    prev_window_table_long  = KBD_longInt;
    prev_window_table_short = KBD_shortInt;
  }

  switch(window_sequence)
  {
  case ONLY_LONG_SEQUENCE:
    ippsMDCTInv_32s_Sfs(p_in_spectrum, samples, p_data->p_mdct_inv_long,
                        -6, p_data->p_buffer_inv);

    ippsMul_32s_Sfs_x(prev_window_table_long, samples,
                    p_out_samples_1st_part, N_LONG/2, 31);

    ippsAdd_32s_ISfs_x(p_in_prev_samples, p_out_samples_1st_part, N_LONG/2, 0);

    ippsMul_32s_Sfs_x(&window_table_long[N_LONG/2], &samples[N_LONG/2],
                    p_out_samples_2nd_part, N_LONG/2, 31);
    break;
  case LONG_START_SEQUENCE:
    ippsMDCTInv_32s_Sfs(p_in_spectrum, samples, p_data->p_mdct_inv_long,
                        -6, p_data->p_buffer_inv);

    ippsMul_32s_Sfs_x(prev_window_table_long, samples,
                    p_out_samples_1st_part, N_LONG/2, 31);

    ippsAdd_32s_ISfs_x(p_in_prev_samples, p_out_samples_1st_part, N_LONG/2, 0);

    ippsCopy_32s_x(&samples[N_LONG / 2], p_out_samples_2nd_part,
                ((3*N_LONG-N_SHORT)/4-N_LONG/2));

    ippsMul_32s_Sfs_x(&window_table_short[N_SHORT / 2],
                    &samples[(3*N_LONG-N_SHORT)/4],
                    &p_out_samples_2nd_part[((3*N_LONG-N_SHORT)/4-N_LONG/2)],
                    N_SHORT/2, 31);

    ippsZero_32s_x(&p_out_samples_2nd_part[(3*N_LONG+N_SHORT)/4-N_LONG/2],
                  N_LONG-(3*N_LONG+N_SHORT)/4);
    break;
  case LONG_STOP_SEQUENCE:
    ippsMDCTInv_32s_Sfs(p_in_spectrum, samples, p_data->p_mdct_inv_long,
                        -6, p_data->p_buffer_inv);

    ippsZero_32s_x(p_out_samples_1st_part, (N_LONG-N_SHORT)/4);

    ippsMul_32s_Sfs_x(prev_window_table_short, &samples[(N_LONG-N_SHORT)/4],
                    &p_out_samples_1st_part[(N_LONG-N_SHORT)/4], N_SHORT/2, 31);

    ippsCopy_32s_x(&samples[(N_LONG+N_SHORT)/4],
                  &p_out_samples_1st_part[(N_LONG + N_SHORT)/4],
                  ((N_LONG/2)-((N_LONG+N_SHORT)/4)));

    ippsAdd_32s_ISfs_x(p_in_prev_samples, p_out_samples_1st_part, N_LONG/2, 0);

    ippsMul_32s_Sfs_x(&window_table_long[N_LONG/2],
                    &samples[N_LONG/2], p_out_samples_2nd_part, N_LONG/2, 31);
    break;
  case EIGHT_SHORT_SEQUENCE:
    p_samples = samples;
    p_spectrum = p_in_spectrum;

    ippsZero_32s_x(p_samples, N_LONG);
    p_samples += (N_LONG - N_SHORT) / 4;
    ippsMDCTInv_32s_Sfs(p_spectrum, (Ipp32s*)mdct_out_short,
                        p_data->p_mdct_inv_short, -3, p_data->p_buffer_inv);

    ippsMul_32s_Sfs_x(prev_window_table_short, mdct_out_short, p_samples, N_SHORT/2, 31);
    ippsMul_32s_Sfs_x(&window_table_short[N_SHORT/2], &mdct_out_short[N_SHORT/2],
                    &p_samples[N_SHORT/2],N_SHORT/2, 31);

    p_samples  += N_SHORT/2;
    p_spectrum += N_SHORT/2;

    for (j = 1; j < 8; j++) {
      ippsMDCTInv_32s_Sfs(p_spectrum, (Ipp32s*)mdct_out_short,
                          p_data->p_mdct_inv_short, -3, p_data->p_buffer_inv);

      ippsMul_32s_ISfs_x(window_table_short, mdct_out_short, N_SHORT, 31);
      ippsAdd_32s_ISfs_x(mdct_out_short, p_samples, N_SHORT, 0);
      p_samples  += N_SHORT/2;
      p_spectrum += N_SHORT/2;
    }

    ippsAdd_32s_Sfs_x(p_in_prev_samples, samples,
                    p_out_samples_1st_part, N_LONG/2, 0);
    ippsCopy_32s_x(&samples[N_LONG/2], p_out_samples_2nd_part, N_LONG/2);
    break;
  }
}

/********************************************************************/

void FilterbankEncInt(sFilterbankInt* p_data,
                      Ipp32s* p_in_samples_1st_part,
                      Ipp32s* p_in_samples_2nd_part,
                      int window_sequence,
                      int window_shape,
                      int prev_window_shape,
                      Ipp32s* p_out_spectrum)
{
  Ipp32s mdct_in[N_LONG];
  Ipp32s *window_table_long, *prev_window_table_long;
  Ipp32s *window_table_short, *prev_window_table_short;

  if (0 == window_shape) {
    window_table_long  = SIN_longInt;
    window_table_short = SIN_shortInt;
  } else {
      window_table_long  = KBD_longInt;
      window_table_short = KBD_shortInt;
  }

  if (0 == prev_window_shape) {
      prev_window_table_long  = SIN_longInt;
      prev_window_table_short = SIN_shortInt;
  } else {
      prev_window_table_long  = KBD_longInt;
      prev_window_table_short = KBD_shortInt;
  }

  switch(window_sequence)
  {
  case ONLY_LONG_SEQUENCE:
    ippsMul_32s_Sfs_x(p_in_samples_1st_part, prev_window_table_long,
                    mdct_in, N_LONG/2, 31);

    ippsMul_32s_Sfs_x(p_in_samples_2nd_part,
                    &window_table_long[N_LONG/2],
                    &mdct_in[N_LONG/2], N_LONG/2, 31);

    ippsMDCTFwd_32s_Sfs(mdct_in, p_out_spectrum, p_data->p_mdct_fwd_long,
                        12, p_data->p_buffer_fwd);
    break;
  case LONG_START_SEQUENCE:
    ippsMul_32s_Sfs_x(p_in_samples_1st_part, prev_window_table_long,
                    mdct_in, N_LONG/2, 31);

    ippsCopy_32s_x(p_in_samples_2nd_part, &mdct_in[1024], 1472-1024);

    ippsMul_32s_Sfs_x(&p_in_samples_2nd_part[1472-1024],
                    &window_table_short[N_SHORT/2],
                    &mdct_in[1472], 1600-1472, 31);

    ippsZero_32s_x(&mdct_in[1600], (2048-1600));

    ippsMDCTFwd_32s_Sfs(mdct_in, p_out_spectrum, p_data->p_mdct_fwd_long,
                        12, p_data->p_buffer_fwd);
    break;
  case LONG_STOP_SEQUENCE:
    ippsZero_32s_x(mdct_in, 448);

    ippsMul_32s_Sfs_x(&p_in_samples_1st_part[448], prev_window_table_short,
                    &mdct_in[448], 576-448, 31);

    ippsCopy_32s_x(&p_in_samples_1st_part[576], &mdct_in[576], 1024-576);

    ippsMul_32s_Sfs_x(p_in_samples_2nd_part,
                    &window_table_long[N_LONG/2],
                    &mdct_in[N_LONG/2], N_LONG/2, 31);

    ippsMDCTFwd_32s_Sfs(mdct_in, p_out_spectrum, p_data->p_mdct_fwd_long,
                        12, p_data->p_buffer_fwd);
    break;
  case EIGHT_SHORT_SEQUENCE:
    ippsMul_32s_Sfs_x(&p_in_samples_1st_part[0], prev_window_table_short,
                    &mdct_in[0], N_SHORT/2, 31);
    ippsMul_32s_Sfs_x(&p_in_samples_1st_part[128], &window_table_short[N_SHORT/2],
                    &mdct_in[N_SHORT/2], N_SHORT/2, 31);

    ippsMDCTFwd_32s_Sfs(mdct_in,
                        p_out_spectrum,
                        p_data->p_mdct_fwd_short,
                        9, p_data->p_buffer_fwd);
    break;
  default:
    break;
  }
}

/********************************************************************/
