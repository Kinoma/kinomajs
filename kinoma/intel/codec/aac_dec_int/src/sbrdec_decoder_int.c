/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2006 Intel Corporation. All Rights Reserved.
//
*/

/********************************************************************/
//***
#include "kinoma_ipp_lib.h"

#include<ipps.h>
#include<ippac.h>
#include<math.h>

#include"sbrdec_api_int.h"
#include"sbrdec_element.h"
#include"sbrdec_tables_int.h"
#include"sbrdec_own_int.h"



#ifdef KINOMA_DEBUG
extern int g_kinoma_debug_on;
#endif

/********************************************************************/

#ifndef ID_SCE
#define ID_SCE    0x0
#endif

#ifndef ID_CPE
#define ID_CPE    0x1
#endif

#ifndef UMC_MAX
#define UMC_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef UMC_MIN
#define UMC_MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

/********************************************************************/

static Ipp32s sbrPassiveUpsampling(Ipp32s* XBuf,
                                   Ipp32s* iZBuf,
                                   Ipp32s mode);

/********************************************************************/

static Ipp32s sbrActiveUpsampling(Ipp32s* XBuf,
                                  Ipp32s* iYBuf,

                                  Ipp32s xoverBand,

                                  Ipp32s* iZBuf,

                                  Ipp32s mode);

/********************************************************************/

/* Set HF subbands to zero */
int sbrCleanHFBand(int** pYBuf, int startBand, int stopBand, int mode);

/********************************************************************/

static int sbrUpDateBands(int** ppSrc );

/********************************************************************/

static int sbrUpdateAmpRes( int bs_frame_class, int L_E, int bs_amp_res)
{
  if ((bs_frame_class == FIXFIX) && (L_E == 1))
    bs_amp_res = 0;

  return bs_amp_res;
}

/********************************************************************/

Ipp32s sbrdecReset( sSbrBlock* pSbr )
{
  IppStatus status;

  sbrdecResetCommon( &(pSbr->sbr_com) );

  // FIXED-POINT
  status = ippsZero_32s_x(pSbr->sbr_WS.iBufGain[0][0],  2*MAX_NUM_ENV*MAX_NUM_ENV_VAL);
  status = ippsZero_32s_x(pSbr->sbr_WS.iBufNoise[0][0], 2*MAX_NUM_ENV*MAX_NUM_ENV_VAL);
  status = ippsZero_32s_x(pSbr->sbr_WS.bwArray[0],      2*MAX_NUM_NOISE_VAL);

  return 0;//OK
}

/********************************************************************/

sSbrBlock *sbrInitDecoder( void )
{
  Ipp32s  ch, j;
  sSbrBlock *pSbr = 0;

  const Ipp32s Size32 = (32) * (NUM_TIME_SLOTS * RATE + SBR_TIME_HFGEN);
  const Ipp32s Size64 = (64) * (NUM_TIME_SLOTS * RATE + SBR_TIME_HFGEN);

  IppStatus status = ippStsNoErr;

  pSbr = (sSbrBlock *) ippsMalloc_8u_x(sizeof(sSbrBlock));

  if ( pSbr == 0 )
    return 0;

/* -------------------------------- set memory for matrix ------------------------------------ */
  for (ch = 0; ch < 2; ch++) {
    pSbr->sbr_WS.iXBuf[ch][0] = ippsMalloc_32s_x((Size32 + Size64) * 2);
    ippsZero_32s_x(pSbr->sbr_WS.iXBuf[ch][0], (Size32 + Size64) * 2);

    /* process need because mixing memory will be done */
    pSbr->sbr_WS._dcMemoryMatrix[ch] = 0;
    pSbr->sbr_WS._dcMemoryMatrix[ch] = pSbr->sbr_WS.iXBuf[ch][0];

    for (j = 0; j < (NUM_TIME_SLOTS * RATE + SBR_TIME_HFGEN); j++) {
      pSbr->sbr_WS.iXBuf[ch][j] = pSbr->sbr_WS.iXBuf[ch][0] + j * (2*32);

      pSbr->sbr_WS.iYBuf[ch][j] = pSbr->sbr_WS.iXBuf[ch][0] + (2*Size32 + 0 * Size64) + j * (2*64);
    }
  }

/* --------------------------------  set default values ------------------------------------ */

  /* FIXED-POINT BLOCK */
  sbrdecReset( pSbr );

  // only once time
  pSbr->sbr_com.sbrHeaderFlagPresent = 0;

/* -------------------------------- malloc memory for general work buffer ---------------------------- */

  if (status == ippStsNoErr)
    return pSbr;        // OK
  //else [dead codes]
  //  return 0;
}

/********************************************************************/

Ipp32s sbrFreeDecoder(sSbrBlock * pSbr)
{
  Ipp32s  ch;

  if( pSbr == 0 )
    return 0;

  for (ch = 0; ch < 2; ch++) {
    if( pSbr->sbr_WS._dcMemoryMatrix[ch] )
      ippsFree_x( (Ipp32s*)(pSbr->sbr_WS._dcMemoryMatrix[ch]) );
  }

  ippsFree_x(pSbr);

  return 0;     // OK
}

/********************************************************************/

Ipp32s sbrGetFrame(Ipp32s *pSrc, Ipp32s *pDst,
                   sSbrBlock * pSbr, sSbrDecFilter* sbr_filter,
                   int ch, int decode_mode, int dwnsmpl_mode,
                   Ipp8u* pWorkBuffer, int* scaleFactor )
{
  sSbrDecCommon* com   = &(pSbr->sbr_com);
  sSbrDecWorkSpace* ws = &(pSbr->sbr_WS);

  int startBand = RATE * com->tE[ch][0];
  int stopBand = 0;
  int transitionBand = pSbr->sbr_com.transitionBand[ch];
  int l;
  int bs_amp_res;
  int my_ch;
  int degPatchedCoefs[64];

  Ipp32s bufZ[2*64];

  Ipp32s* bufDelay = (dwnsmpl_mode == HEAAC_DWNSMPL_OFF) ?
          ws->SynthesisBufDelay[ch] : ws->SynthesisDownBufDelay[ch];

  int xScale = (dwnsmpl_mode == HEAAC_DWNSMPL_OFF) ? 64 : 32;

/* -------------------------------- Analysis ---------------------------- */
  if (HEAAC_HQ_MODE == decode_mode) {

    sbrAnalysisFilter_SBR_RToC_32s_D2L_Sfs(pSrc,
                                            (Ipp32sc**)(ws->iXBuf[ch]) + SBR_TIME_HFGEN,
                                            32, com->kx,
                                            sbr_filter->pFFTSpecQMFA,
                                            ws->AnalysisBufDelay[ch],
                                            *scaleFactor,
                                            pWorkBuffer);


  }else{ //if (HEAAC_LP_MODE == decode_mode)
    for(l=0; l<NUM_TIME_SLOTS * RATE; l++) {

      /*********************************************************************
       * NOTE:
       * (1) matrix idea is better than 1D
       *     but if you can use this function for non-matrix,
       *     you must use such consrtuctions
       * (2) you can use non-standard function of window and get better
       *     result (speech codec or other area)
       *********************************************************************/
       sbrAnalysisFilter_SBR_RToR_32s_D2L_Sfs(pSrc + l*32,
                                          ws->iXBuf[ch] + SBR_TIME_HFGEN + l,
                                          1, com->kx,
                                          sbr_filter->pFFTSpecQMFA_LPmode,
                                          ws->AnalysisBufDelay[ch],
                                          *scaleFactor,
                                          pWorkBuffer);

    }
  }

/* -------------------------------- <SBR process> ---------------------------- */
  if ((com->sbrHeaderFlagPresent != 0) && (com->sbrFlagError == 0) &&
      ((com->id_aac == ID_SCE) || (com->id_aac == ID_CPE))) {

        stopBand = RATE * com->tE[ch][ com->L_E[ch] ];

        /* skip couple channel for ch == R == 1 */
        if ( !ch || !com->bs_coupling )
        {
          bs_amp_res = sbrUpdateAmpRes( com->bs_frame_class[ch], com->L_E[ch], com->bs_amp_res);
          sbrDequantization( com, ws, ws->sfsEOrig[ch], ch, bs_amp_res );
        }

        /* Set HF subbands to zero */
        sbrCleanHFBand(ws->iYBuf[ch], startBand, stopBand, decode_mode);

        sbrGenerationHF(ws->iXBuf[ch],
                        ws->iYBuf[ch],
                        &(pSbr->sbr_com), ws->bwArray[ch],
                        degPatchedCoefs, ch, decode_mode);

#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
		{
			int *p1 = ws->iXBuf[ch][8 + SBR_TIME_HFADJ];
			int *p2 = ws->iYBuf[ch][8 + SBR_TIME_HFADJ];
			fprintf( stderr, "   kinoma debug: o78:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p1[0],p1[1],p1[2],p1[3],p1[4],p1[5] );
			fprintf( stderr, "   kinoma debug: o79:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p2[0],p2[1],p2[2],p2[3],p2[4],p2[5] );
		}
#endif

        if (HEAAC_LP_MODE == decode_mode) {
          for (l = startBand; l < stopBand; l++) {
            ippsZero_32s_x(ws->iYBuf[ch][SBR_TIME_HFADJ + l], com->kx);
          }
        }

    my_ch = (com->bs_coupling == 1) ? 0 : ch;

    sbrAdjustmentHF(
                    ws->iYBuf[ch],
                    ws->vEOrig[ch], ws->vNoiseOrig[ch],
                    ws->sfsEOrig[my_ch],
                    ws->iBufGain[ch], ws->iBufNoise[ch],
                    &(pSbr->sbr_com), degPatchedCoefs, pWorkBuffer,
                    com->Reset, ch, decode_mode );


    for (l = 0; l < NUM_TIME_SLOTS * RATE; l++) {

      int xoverBand = ( l < transitionBand ) ? com->kx_prev : com->kx;

#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
		{
			int *p1 = ws->iXBuf[ch][l + SBR_TIME_HFADJ];
			int *p2 = ws->iYBuf[ch][l + SBR_TIME_HFADJ];
			fprintf( stderr, "   kinoma debug: o96:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p1[0],p1[1],p1[2],p1[3],p1[4],p1[5] );
			fprintf( stderr, "   kinoma debug: o97:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p2[0],p2[1],p2[2],p2[3],p2[4],p2[5] );
		}
#endif

      sbrActiveUpsampling(ws->iXBuf[ch][l + SBR_TIME_HFADJ],
                          ws->iYBuf[ch][l + SBR_TIME_HFADJ],
                          xoverBand,
                          bufZ, decode_mode);

#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
		{
			int *p = bufZ;
			fprintf( stderr, "   kinoma debug: o98:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p[0],p[1],p[2],p[3],p[4],p[5] );
		}
#endif


      /* may be is used pointer to fuction instead CASE */
      sbrSynthesisFilter_32s(bufZ, pDst + xScale*l,
                             bufDelay, sbr_filter,
                             pWorkBuffer, scaleFactor,
                             decode_mode | dwnsmpl_mode );

#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
		{
			int *p = pDst + xScale*l;
			fprintf( stderr, "   kinoma debug: o99:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p[0],p[1],p[2],p[3],p[4],p[5] );
		}
#endif

    }

    /* UpDate High Band */
    sbrUpDateBands(ws->iYBuf[ch]);


// store ch independ
    com->transitionBand[ch] = stopBand - NUM_TIME_SLOTS * RATE;

  } else {

    //--------------
    for (l = 0; l < NUM_TIME_SLOTS * RATE; l++) {

      sbrPassiveUpsampling(ws->iXBuf[ch][l + SBR_TIME_HFADJ],
                           bufZ, decode_mode);

      /* may be is use pointer to fuction instead CASE */
      sbrSynthesisFilter_32s(bufZ, pDst + xScale*l,
                             bufDelay, sbr_filter,
                             pWorkBuffer, scaleFactor,
                             decode_mode | dwnsmpl_mode );

    }
    //--------------
  }

   /* UpDate Low Band */
   sbrUpDateBands( ws->iXBuf[ch] );

/* ---------------------------- <store ch depend> ---------------------------- */

  if ((com->id_aac == ID_SCE) || ((com->id_aac == ID_CPE) && (ch == 1))) {
    com->kx_prev = com->kx;
    com->M_prev = com->M;
    com->Reset = 0;
  }

  return 0;     // OK
}

/********************************************************************/

Ipp32s sbrPassiveUpsampling(Ipp32s* iXBuf,
                            Ipp32s* iZBuf,

                            Ipp32s mode)
{
  int xScale = (HEAAC_LP_MODE == mode) ? 1 : 2;

  ippsCopy_32s_x(iXBuf, iZBuf, NUM_TIME_SLOTS * RATE * xScale);
  ippsZero_32s_x( iZBuf + 32 * xScale, NUM_TIME_SLOTS * RATE * xScale);

  return 0;     // OK
}

/********************************************************************/

Ipp32s sbrActiveUpsampling( Ipp32s* iXBuf,
                            Ipp32s* iYBuf,

                            Ipp32s xoverBand,

                            Ipp32s* iZBuf,

                            Ipp32s mode)
{
  Ipp32s  k;
  int xScale = (HEAAC_LP_MODE == mode) ? 1 : 2;

  for (k = 0; k < xoverBand * xScale; k++) {

    iZBuf[k] = iXBuf[k];
  }

  if (mode == HEAAC_LP_MODE) {
    iZBuf[xoverBand - 1]   += iYBuf[xoverBand - 1];
  }

  for (k = xoverBand * xScale; k < 64 * xScale; k++) {
    iZBuf[k] = iYBuf[k];
  }

  return 0;     // OK
}

/********************************************************************/

/* Set HF subbands to zero */
int sbrCleanHFBand(int** pYBuf, int startBand, int stopBand, int mode)
{
  int i;
  int xScale = (HEAAC_LP_MODE == mode) ? 1 : 2;

  for (i = startBand; i < stopBand; i++) {
    ippsZero_32s_x(pYBuf[i + SBR_TIME_HFADJ], 64 * xScale);
  }

  return 0; //OK
}

/********************************************************************/

int sbrUpDateBands(int** ppSrc )
{
  int l;
  int* pBufTmp;

  for (l = 0; l < SBR_TIME_HFGEN; l++) {
    // Re part
    pBufTmp    = ppSrc[l];
    ppSrc[l] = ppSrc[NUM_TIME_SLOTS * RATE + l];
    ppSrc[NUM_TIME_SLOTS * RATE + l] = pBufTmp;
  }

  return 0;
}

/********************************************************************/

/* EOF */
