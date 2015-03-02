/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __SBRDEC_API_INT_H__
#define __SBRDEC_API_INT_H__

#include "ippac.h"
#include "ipps.h"

#include "sbrdec.h"
#include "sbrdec_setting_int.h"

#include "aaccmn_chmap.h"


typedef struct { // specification Filter

  IppsFFTSpec_C_32sc*  pFFTSpecQMFA;
  IppsFFTSpec_C_32sc* pFFTSpecQMFA_LPmode;

  IppsFFTSpec_C_32sc* pFFTSpecQMFS_LPmode;

  IppsFFTSpec_C_32sc* pFFTSpecQMFS;
  IppsFFTSpec_C_32sc* pFFTSpecQMFSD;

  Ipp8u*              pMemSpecQMFA;
  Ipp8u*              pMemSpecQMFA_LPmode;
  Ipp8u*              pMemSpecQMFS;
  Ipp8u*              pMemSpecQMFSD;

} sSbrDecFilter;

typedef struct { //  sbr matrix's work space

/* main-matrix
 *
 * UMC name | ISO name
 *  Xbuf    |   Xlow
 *  YBuf    |   Xhigh & Y - identity
 *  Zbuf    |     X
 *
 * Please, to watch closely
 */
  Ipp32s*  iXBuf[2][40];
  //Ipp32s*  iXBufIm[2][40];

  Ipp32s*  iYBuf[2][40];
  //Ipp32s*  iYBufIm[2][40];

  /*
   * Zbuf - will be used by vector's state
  */
  //Ipp32s*  iZBufRe[2][40];
  //Ipp32s*  iZBufIm[2][40];

  /* these descriptors contain pointer to the allocatable memory (main matrix).
  /  It is need, because mixing wiil be done
  */
  Ipp32s* _dcMemoryMatrix[2];

/* HF adjustment: these buffers keep Noise & Gain data */

  int  iBufGain[2][MAX_NUM_ENV][MAX_NUM_ENV_VAL];
  int  iBufNoise[2][MAX_NUM_ENV][MAX_NUM_ENV_VAL];

  Ipp32s vEOrig[2][5*64];
  Ipp32s sfsEOrig[2][5];
  Ipp32s vNoiseOrig[2][2*5];

  /* for HF generation */
  Ipp32s  bwArray[2][MAX_NUM_NOISE_VAL];

/* external buffer for delay of A, S & SD FILTERs */
  Ipp32s  AnalysisBufDelay[2][320];
  Ipp32s  SynthesisBufDelay[2][1280];
  Ipp32s  SynthesisDownBufDelay[2][1280];

} sSbrDecWorkSpace;

typedef struct {
  sSbrDecCommon    sbr_com;
  sSbrDecWorkSpace sbr_WS;

} sSbrBlock;


#ifdef  __cplusplus
extern  "C" {
#endif

 /* algorithm */

  Ipp32s sbrDequantization( sSbrDecCommon* pSbrCom, sSbrDecWorkSpace* pSbrWS,
                            int* sfsEnv, Ipp32s ch, int bs_amp_res );

  Ipp32s sbrGenerationHF(Ipp32s** iXBuf,
                         Ipp32s** iYBuf,

                         sSbrDecCommon* sbr_com, Ipp32s* bwArray, Ipp32s* degPatched,
                         Ipp32s ch, int decode_mode);

  void sbrAdjustmentHF(
                       Ipp32s**  iYBuf,
                       Ipp32s* vEOrig, Ipp32s* vNoiseOrig,
                       Ipp32s* sfsEOrig,

                       int    iBufGain[][MAX_NUM_ENV_VAL],int    iBufNoise[][MAX_NUM_ENV_VAL],

                       sSbrDecCommon* sbr_com, Ipp32s *degPatched, Ipp8u *WorkBuffer,
                       Ipp32s reset, Ipp32s ch, Ipp32s decode_mode);

  int sbrSynthesisFilter_32s(Ipp32s* bufZRe, Ipp32s* pDst,
                           Ipp32s* bufDelay, sSbrDecFilter* pQmfBank,
                           Ipp8u* pWorkBuf, int* scaleFactor, int mode);


/* SBR GENERAL HIGH LEVEL API: FIXED POINT VERSION */
 // init()
  Ipp32s sbrdecInitFilter( sSbrDecFilter** pDC, int* vecSizeWorkBuf );
  sSbrBlock *sbrInitDecoder( void );

 // reset()
  Ipp32s sbrdecReset( sSbrBlock* pSbr );

 // get_frame()
Ipp32s sbrGetFrame(Ipp32s *pSrc, Ipp32s *pDst,
                   sSbrBlock * pSbr, sSbrDecFilter* sbr_filter,
                   Ipp32s ch, Ipp32s decode_mode,
                   Ipp32s dwnsmpl_mode, Ipp8u* pWorkBuffer, int* scaleFactor );

 // free()
  Ipp32s sbrdecFreeFilter( sSbrDecFilter* pDC );
  Ipp32s sbrFreeDecoder(sSbrBlock * pDst);
/* end */

#ifdef  __cplusplus
}
#endif

#endif             /* __SBRDEC_API_INT_H__ */
/* EOF */
