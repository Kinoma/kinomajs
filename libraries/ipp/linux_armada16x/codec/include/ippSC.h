/***************************************************************************************** 
Copyright (c) 2009, Marvell International Ltd. 
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Marvell nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY MARVELL ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MARVELL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************************/


#ifndef _IPPSC_H_
#define _IPPSC_H_

#include "ippdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== IPP Speech Bit Rate Definition ===================== */

typedef enum {
	IPP_SPCHBR_4750,
	IPP_SPCHBR_5150,
	IPP_SPCHBR_5900,
	IPP_SPCHBR_6700,
	IPP_SPCHBR_7400,
	IPP_SPCHBR_7950,
	IPP_SPCHBR_10200,
	IPP_SPCHBR_12200,
	IPP_SPCHBR_DTX,
	IPP_SPCHBR_5300,
	IPP_SPCHBR_6300,
	IPP_SPCHBR_6600,
	IPP_SPCHBR_8850,
	IPP_SPCHBR_12650,
	IPP_SPCHBR_14250,
	IPP_SPCHBR_15850,
	IPP_SPCHBR_18250,
	IPP_SPCHBR_19850,
	IPP_SPCHBR_23050,
	IPP_SPCHBR_23850
} IppSpchBitRate;

/* ========================== Structure Definition ========================= */

typedef struct {
	Ipp16s	adaptEnableFlag;		/* adaptation enable flag */
	Ipp32s	prevFltEnergy;			/* previous filtered signal energy */
	Ipp32s	prevNoiseLevel;			/* previous noise level */
	Ipp16s	hangoverCount;			/* count for hangover frames */
	Ipp16s	vadVoiceCount;			/* count for VAD voice frames */
} IppG723AVadState;

typedef struct {
	Ipp16s	sidLpcAutoCorrExp;		/* exponent of the autocorrelation */
									/* coefficient of SID LPC coefficients */
	Ipp16s	sidLpcAutoCorr[11];		/* SID LPC coefficients autocorrelation */
	Ipp16s	qSidGainIndex;			/* quantized SID gain index */
	Ipp16s	residualEnergy[3];		/* residual energy of previous 3 frames */
	Ipp16s	sumFrame;				/* summation frames number */
} IppG723ADtxState;

typedef struct {
	Ipp16s	pPrevSignalSublevel[9];	/* signal sublevel vector */
	Ipp16s	pPrevSignalLevel[9];	/* previous signal level vector */
	Ipp16s	pPrevAverageLevel[9];	/* average signal level */
	Ipp16s	pBkgNoiseEstimate[9];	/* background noise estimate vector */
	Ipp16s	pFifthFltState[6];		/* 5th order filters history */
	Ipp16s 	pThirdFltState[5];		/* 3rd order filters history */
	Ipp16s	burstCount;				/* burst counter */
	Ipp16s	hangCount;				/* hangover counter */
	Ipp16s	statCount;				/* stationarity counter */
	Ipp16s	vadReg;					/* intermediate vad decision */
	Ipp16s	complexHigh;			/* high for complex signal decision */	
	Ipp16s	complexLow;				/* low for ccomplex signal decision */
	Ipp16s	complexHangTimer;		/* complex hangover timer */
	Ipp16s	complexHangCount;		/* complex hangover counter */
	Ipp16s	complexWarning;			/* complex_warining flag */
	Ipp16s	corrHp;					/* High-pass filtered signal correlation */
	Ipp16s	pitchFlag;				/* pitch flag by pitch detection */
} IppGSMAMRVad1State;

typedef struct {
	Ipp32s	pEngyEstimate[16];		/* channel energy estimate vector of */
									/* current half-frame */
	Ipp32s	pNoiseEstimate[16];		/* channel noise estimiate vector of */
									/* current half-frame */
	Ipp16s	pLongTermEngyDb[16];	/* channel long-term log energy vector */ 
									/* of current half-frame */
	Ipp16s	preEmphasisFactor;		/* pre-emphasize factor */
	Ipp16s	updateCount;			/* update counter */
	Ipp16s	lastUpdateCount;		/* last update counter */
	Ipp16s	hysterCount;			/* hyster counter */
	Ipp16s	prevNormShift;			/* previous normalize shift bits */
	Ipp16s	shiftState;				/* previous half-frame shift state */	
	Ipp16s	forcedUpdateFlag;		/* forced update flag */
	Ipp16s	ltpSnr;					/* long-term peak SNR */
	Ipp16s	variabFactor;			/* background noise variablity factor */
	Ipp16s	negSnrBias;				/* negative SNR sensitivity Bias */
	Ipp16s	burstCount;				/* burst counter */
	Ipp16s	hangOverCount;			/* hangover counter */
	Ipp32s	frameCount;				/* frame counter */
} IppGSMAMRVad2State;

/* ========================== G.723.1/A Functions ========================== */

/* LP analysis */
IPPAPI(IppStatus, ippsAutoCorr_G723_16s,
	(const Ipp16s * pSrcSpch, Ipp16s * pResultAutoCorrExp, 
	Ipp16s * pDstAutoCorr))

IPPAPI(IppStatus, ippsLevinsonDurbin_G723_16s,
	(const Ipp16s * pSrcAutoCorr, Ipp16s * pValResultSineDtct, 
	Ipp16s * pResultResidualEnergy, Ipp16s * pDstLpc))

IPPAPI(IppStatus, ippsLPCToLSF_G723_16s,
	(const Ipp16s * pSrcLpc, const Ipp16s * pSrcPrevLsf, Ipp16s * pDstLsf))

IPPAPI(IppStatus, ippsLSFToLPC_G723_16s,
	(const Ipp16s * pSrcLsf, Ipp16s * pDstLpc))

IPPAPI(IppStatus, ippsLSFQuant_G723_16s32s,
	(const Ipp16s * pSrcLsf, const Ipp16s * pSrcPrevLsf, 
	Ipp32s * pResultQLsfIndex))

/* adaptive-codebook search */
IPPAPI(IppStatus, ippsOpenLoopPitchSearch_G723_16s,
	(const Ipp16s * pSrcWgtSpch, Ipp16s * pResultOpenDelay))

IPPAPI(IppStatus, ippsHarmonicSearch_G723_16s,
	(Ipp16s valOpenDelay, const Ipp16s * pSrcWgtSpch, 
	Ipp16s * pResultHarmonicDelay, Ipp16s * pResultHarmonicGain))

IPPAPI(IppStatus, ippsAdaptiveCodebookSearch_G723,
	(Ipp16s valBaseDelay, const Ipp16s * pSrcAdptTarget, 
	const Ipp16s * pSrcImpulseResponse, const Ipp16s * pSrcPrevExcitation, 
	const Ipp32s * pSrcPrevError, Ipp16s * pResultCloseLag, 
	Ipp16s * pResultAdptGainIndex, Ipp16s subFrame, Ipp16s sineDtct, 
	IppSpchBitRate bitRate))

IPPAPI(IppStatus, ippsDecodeAdaptiveVector_G723_16s,
	(Ipp16s valBaseDelay, Ipp16s valCloseLag, Ipp16s valAdptGainIndex,
	const Ipp16s * pSrcPrevExcitation, Ipp16s * pDstAdptVector,
	IppSpchBitRate bitRate))

/* fixed-codebook search */
IPPAPI(IppStatus, ippsToeplizMatrix_G723_16s,
	(const Ipp16s * pSrcImpulseResponse, Ipp16s * pDstMatrix))

IPPAPI(IppStatus, ippsMPMLQFixedCodebookSearch_G723,
	(Ipp16s valBaseDelay, const Ipp16s * pSrcImpulseResponse, 
	const Ipp16s * pSrcResidualTarget, Ipp16s * pDstFixedVector, 
	Ipp16s * pResultGrid, Ipp16s * pResultTrainDirac, 
	Ipp16s * pResultAmpIndex, Ipp16s * pResultAmplitude, 
	Ipp32s * pResultPosition,  Ipp16s subFrame))

IPPAPI(IppStatus, ippsACELPFixedCodebookSearch_G723_16s,
	(const Ipp16s * pSrcFixedCorr, const Ipp16s * pSrcMatrix, 
	Ipp16s * pDstFixedSign, Ipp16s * pDstFixedPosition, Ipp16s * pResultGrid, 
	Ipp16s * pDstFixedVector, Ipp16s * pSearchTimes))

/* filtering */
IPPAPI(IppStatus, ippsSynthesisFilter_G723_16s,
	(const Ipp16s * pSrcLpc, const Ipp16s * pSrcResidual, 
	Ipp16s * pSrcDstIIRState, Ipp16s * pDstSpch))

IPPAPI(IppStatus, ippsPitchPostFilter_G723_16s,
	(Ipp16s valBaseDelay, const Ipp16s * pSrcResidual, Ipp16s * pResultDelay, 
	Ipp16s * pResultPitchGain, Ipp16s * pResultScalingGain, Ipp16s subFrame, 
	IppSpchBitRate bitRate))

/* ============================ GSM-AMR Functions ========================== */

/* LP analysis */
IPPAPI(IppStatus, ippsAutoCorr_GSMAMR_16s32s,
	(const Ipp16s * pSrcSpch, Ipp32s * pDstAutoCorr, IppSpchBitRate mode))

IPPAPI(IppStatus, ippsLevinsonDurbin_GSMAMR,
	(const Ipp32s * pSrcAutoCorr, Ipp16s * pSrcDstLpc))

IPPAPI(IppStatus, ippsLPCToLSP_GSMAMR_16s,
	(const Ipp16s * pSrcLpc, const Ipp16s * pSrcPrevLsp, Ipp16s * pDstLsp))

IPPAPI(IppStatus, ippsLSPToLPC_GSMAMR_16s,
	(const Ipp16s * pSrcLsp, Ipp16s * pDstLpc))

IPPAPI(IppStatus, ippsLSPQuant_GSMAMR_16s,
	(const Ipp16s * pSrcLsp, Ipp16s * pSrcDstPrevQLsfResidual, 
	Ipp16s * pDstQLsp, Ipp16s * pDstQLspIndex, IppSpchBitRate mode))

/* adaptive-codebook search */
IPPAPI(IppStatus, ippsImpulseResponseTarget_GSMAMR_16s,
	(const Ipp16s * pSrcSpch, const Ipp16s * pSrcWgtLpc1, 
	const Ipp16s * pSrcWgtLpc2, const Ipp16s * pSrcQLpc, 
	const Ipp16s * pSrcSynFltState, const Ipp16s * pSrcWgtFltState,  
	Ipp16s * pDstImpulseResponse, Ipp16s * pDstLpResidual, 
	Ipp16s * pDstAdptTarget))

IPPAPI(IppStatus, ippsOpenLoopPitchSearchNonDTX_GSMAMR_16s,
	(const Ipp16s * pSrcWgtLpc1, const Ipp16s * pSrcWgtLpc2,
	const Ipp16s * pSrcSpch, Ipp16s * pValResultPrevMidPitchLag, 
	Ipp16s * pValResultVvalue, Ipp16s * pSrcDstPrevPitchLag, 
	Ipp16s * pSrcDstPrevWgtSpch, Ipp16s * pDstOpenLoopLag, 
	Ipp16s * pDstOpenLoopGain, IppSpchBitRate mode))

IPPAPI(IppStatus, ippsOpenLoopPitchSearchDTXVAD1_GSMAMR_16s,
	(const Ipp16s * pSrcWgtLpc1, const Ipp16s * pSrcWgtLpc2, 
	const Ipp16s * pSrcSpch, Ipp16s * pValResultToneFlag, 
	Ipp16s * pValResultPrevMidPitchLag, Ipp16s * pValResultVvalue, 
	Ipp16s * pSrcDstPrevPitchLag, Ipp16s * pSrcDstPrevWgtSpch, 
	Ipp16s * pResultMaxHpCorr, Ipp16s * pDstOpenLoopLag, 
	Ipp16s * pDstOpenLoopGain, IppSpchBitRate mode))

IPPAPI(IppStatus, ippsOpenLoopPitchSearchDTXVAD2_GSMAMR,
	(const Ipp16s * pSrcWgtLpc1, const Ipp16s * pSrcWgtLpc2, 
	const Ipp16s * pSrcSpch, Ipp16s * pValResultPrevMidPitchLag,
	Ipp16s * pValResultVvalue, Ipp16s * pSrcDstPrevPitchLag, 
	Ipp16s * pSrcDstPrevWgtSpch, Ipp32s * pResultMaxCorr, 
	Ipp32s * pResultWgtEnergy, Ipp16s * pDstOpenLoopLag, 
	Ipp16s * pDstOpenLoopGain, IppSpchBitRate mode))

IPPAPI(IppStatus, ippsAdaptiveCodebookSearch_GSMAMR_16s,
	(const Ipp16s * pSrcTarget, const Ipp16s * pSrcImpulseResponse, 
	const Ipp16s * pSrcOpenLoopLag, Ipp16s * pValResultPrevIntPitchLag,
	Ipp16s * pSrcDstExcitation, Ipp16s * pResultFracPitchLag, 
	Ipp16s * pResultAdptIndex, Ipp16s * pDstAdptVector, Ipp16s subFrame, 
	IppSpchBitRate mode))

IPPAPI(IppStatus, ippsAdaptiveCodebookDecode_GSMAMR_16s,
	(Ipp16s valAdptIndex, Ipp16s * pValResultPrevIntPitchLag, 
	Ipp16s * pValResultLtpLag, Ipp16s * pSrcDstExcitation, 
	Ipp16s * pResultIntPitchLag, Ipp16s * pDstAdptVector, Ipp16s subFrame, 
	Ipp16s bfi, Ipp16s inBackgroundNoise, Ipp16s voicedHangover,
	IppSpchBitRate mode))

/* fixed-codebook search */
IPPAPI(IppStatus, ippsAlgebraicCodebookSearch_GSMAMR_16s,
	(Ipp16s	valIntPitchLag,	Ipp16s valBoundQAdptGain, 
	const Ipp16s * pSrcFixedTarget, const Ipp16s * pSrcLtpResidual,
	Ipp16s * pSrcDstImpulseResponse, Ipp16s	* pDstFixedVector, 
	Ipp16s * pDstFltFixedVector, Ipp16s * pDstEncPosSign, 
	Ipp16s subFrame, IppSpchBitRate mode))

IPPAPI(IppStatus, ippsAlgebraicCodebookSearchEX_GSMAMR_16s,
	(Ipp16s	valIntPitchLag,	Ipp16s valBoundQAdptGain, 
	const Ipp16s * pSrcFixedTarget, const Ipp16s * pSrcLtpResidual,
	Ipp16s * pSrcDstImpulseResponse, Ipp16s	* pDstFixedVector, 
	Ipp16s * pDstFltFixedVector, Ipp16s * pDstEncPosSign, 
	Ipp16s subFrame, IppSpchBitRate mode, Ipp32s * pBuffer))

/* parameter decode */
IPPAPI(IppStatus, ippsQuantLSPDecode_GSMAMR_16s,
	(const Ipp16s * pSrcQLspIndex, Ipp16s * pSrcDstPrevQLsfResidual,
	Ipp16s * pSrcDstPrevQLsf, Ipp16s * pSrcDstPrevQLsp, Ipp16s * pDstQLsp, 
	Ipp16s bfi, IppSpchBitRate mode))

IPPAPI(IppStatus, ippsFixedCodebookDecode_GSMAMR_16s,
	(const Ipp16s * pSrcFixedIndex, Ipp16s * pDstFixedVector,
	Ipp16s subFrame, IppSpchBitRate	mode))

/* filtering */
IPPAPI(IppStatus, ippsPostFilter_GSMAMR_16s,
	(const Ipp16s * pSrcQLpc, const Ipp16s * pSrcSpch, 
	Ipp16s * pValResultPrevResidual, Ipp16s * pValResultPrevScalingGain, 
	Ipp16s * pSrcDstFormantFIRState, Ipp16s * pSrcDstFormantIIRState, 
	Ipp16s * pDstFltSpch, IppSpchBitRate mode))

/* voice activity detector/discontinuous transmission */
IPPAPI(IppStatus, ippsVAD1_GSMAMR_16s,
	(const Ipp16s * pSrcSpch, IppGSMAMRVad1State * pValResultVad1State,	
	Ipp16s * pResultVadFlag, Ipp16s	maxHpCorr, Ipp16s toneFlag))

IPPAPI(IppStatus, ippsVAD2_GSMAMR_16s,
	(const Ipp16s * pSrcSpch, IppGSMAMRVad2State * pValResultVad2State,
	Ipp16s * pResultVadFlag, Ipp16s ltpFlag))

IPPAPI(IppStatus, ippsEncDTXSID_GSMAMR_16s,
	(const Ipp16s * pSrcLspBuffer, const Ipp16s * pSrcLogEnergyBuffer, 
	Ipp16s * pValResultLogEnergyIndex, Ipp16s * pValResultDtxLsfRefIndex, 
	Ipp16s * pSrcDstDtxQLsfIndex, Ipp16s * pSrcDstPredQErr, 
	Ipp16s * pSrcDstPredQErrMR122, Ipp16s sidFlag))

IPPAPI(IppStatus, ippsEncDTXHandler_GSMAMR_16s,
	(Ipp16s * pValResultHangOverCount, Ipp16s * pValResultDtxElapseCount, 
	Ipp16s * pValResultUsedMode, Ipp16s * pResultSidFlag, Ipp16s vadFlag))

IPPAPI(IppStatus, ippsEncDTXBuffer_GSMAMR_16s,
	(const Ipp16s * pSrcSpch, const Ipp16s * pSrcLsp, 
	Ipp16s * pValResultUpdateIndex, Ipp16s * pSrcDstLspBuffer, 
	Ipp16s * pSrcDstLogEnergyBuffer))

IPPAPI(IppStatus, ippsDecDTXBuffer_GSMAMR_16s, 
	(const Ipp16s * pSrcSpch, const Ipp16s * pSrcLsf, 
	Ipp16s * pValResultUpdateIndex, Ipp16s * pSrcDstLsfBuffer, 
	Ipp16s * pSrcDstLogEnergyBuffer))

/* ==================== G.729 IPP definitions ======================== */
/* ======== LP Analysis ===========*/
IPPAPI(IppStatus,ippsAutoCorr_G729B,
       (const Ipp16s* pSrcSpch, Ipp16s* pResultAutoCorrExp, Ipp32s* pDstAutoCorr))
IPPAPI(IppStatus,ippsLevinsonDurbin_G729B,(const Ipp32s * pSrcAutoCorr, 
       Ipp16s * pDstLPC, Ipp16s * pDstRc, Ipp16s * pResultResidualEnergy))
IPPAPI( IppStatus, ippsLPCToLSP_G729_16s, 
       (const Ipp16s* pLPC, const Ipp16s* pSrcPrevLsp, Ipp16s* pLSP) )
IPPAPI( IppStatus, ippsLPCToLSP_G729A_16s, 
       (const Ipp16s* pLPC, const Ipp16s* pSrcPrevLsp, Ipp16s* pLSP) )
IPPAPI( IppStatus, ippsLSFToLSP_G729_16s, (const Ipp16s *pLSF, Ipp16s *pLSP) )
IPPAPI( IppStatus, ippsLSFQuant_G729_16s, (const Ipp16s *pLSF, Ipp16s *pQuantLSFTable, 
        Ipp16s *pQuantLSF, Ipp16s *quantIndex) )

IPPAPI( IppStatus, ippsLSFDecode_G729_16s, (const Ipp16s *quantIndex, Ipp16s *pQuantLSPTable, 
       Ipp16s *pQuantLSF) )
IPPAPI( IppStatus, ippsLSFDecode_G729B_16s, (const Ipp16s *quantIndex, Ipp16s *pQuantLSFTable, 
        Ipp16s *pQuantLSF) )
IPPAPI( IppStatus, ippsLSFDecodeErased_G729_16s, (Ipp16s maIndex, Ipp16s *pQuantLSFTable, 
       Ipp16s *pQuantLSF) )
IPPAPI( IppStatus, ippsLSPToLPC_G729_16s, (const Ipp16s *pSrcLSP, Ipp16s *pDstLPC) )

IPPAPI( IppStatus, ippsLSPQuant_G729_16s, (const Ipp16s * pSrcLsp, Ipp16s * pSrcDstPrevFreq,
        Ipp16s * pDstQLsp, Ipp16s * pDstQLspIndex))
IPPAPI( IppStatus, ippsLSPToLSF_G729_16s, (const Ipp16s *pLSP, Ipp16s *pLSF))
IPPAPI( IppStatus, ippsLSPToLSF_Norm_G729_16s, (const Ipp16s *pLSP, Ipp16s *pLSF) )
IPPAPI( IppStatus, ippsLagWindow_G729_32s_I, (Ipp32s *pSrcDst, int len) )      
/*=============== Adaptive Codebook Search ============ */
IPPAPI( IppStatus, ippsAdaptiveCodebookSearch_G729_16s, (Ipp16s valOpenDelay,
        const Ipp16s * pSrcAdptTarget, const Ipp16s * pSrcImpulseResponse,
        Ipp16s * pSrcDstPrevExcitation, Ipp16s * pDstDelay, 
        Ipp16s * pDstAdptVector, Ipp16s subFrame))
IPPAPI( IppStatus, ippsAdaptiveCodebookSearch_G729A_16s, (Ipp16s valOpenDelay,
        const Ipp16s * pSrcAdptTarget, const Ipp16s * pSrcImpulseResponse,
        Ipp16s * pSrcDstPrevExcitation, Ipp16s * pDstDelay, 
        Ipp16s * pDstAdptVector, Ipp16s subFrame))
IPPAPI( IppStatus, ippsDecodeAdaptiveVector_G729_16s_I,(const Ipp16s * pSrcDelay, 
       Ipp16s * pSrcDstPrevExcitation))
IPPAPI( IppStatus, ippsAdaptiveCodebookGain_G729_16s, (const Ipp16s * pSrcAdptTarget, 
       const Ipp16s * pSrcImpulseResponse, const Ipp16s * pSrcAdptVector, 
       Ipp16s * pDstFltAdptVector, Ipp16s * pResultAdptGain))
IPPAPI( IppStatus, ippsAdaptiveCodebookGain_G729A_16s, (const Ipp16s * pSrcAdptTarget,
       const Ipp16s * pSrcLPC, const Ipp16s * pSrcAdptVector, Ipp16s * pDstFltAdptVector, 
       Ipp16s * pResultAdptGain)) 
IPPAPI(IppStatus,ippsAdaptiveCodebookContribution_G729_16s, (Ipp16s gain, 
       const Ipp16s *pFltAdptVector, const Ipp16s *pSrcAdptTarget, Ipp16s* pDstAdptTarget))
/* ========== Open loop pitch search ========= */
IPPAPI( IppStatus, ippsOpenLoopPitchSearch_G729_16s, (const Ipp16s *pSrc, Ipp16s* bestLag) )
IPPAPI( IppStatus, ippsOpenLoopPitchSearch_G729A_16s, (const Ipp16s *pSrc, Ipp16s* bestLag) )
/* ============== Fixed Codebook Search ================= */
IPPAPI( IppStatus, ippsFixedCodebookSearch_G729_32s16s, (const Ipp16s *pSrcFixedCorr, 
       Ipp32s *pSrcDstMatrix, Ipp16s *pDstFixedVector,
       Ipp16s *pDstFixedIndex, Ipp16s *pSearchTimes, Ipp16s subFrame))
IPPAPI(IppStatus,ippsFixedCodebookSearch_G729A_32s16s,(const Ipp16s *pSrcFixedCorr,
       Ipp32s *pSrcDstMatrix, Ipp16s *pDstFixedVector, Ipp16s *pDstFixedIndex))
IPPAPI( IppStatus, ippsToeplizMatrix_G729_16s32s, (const Ipp16s *pSrc, Ipp32s *pDst) )
/* ================= Gain functions =============== */
IPPAPI(IppStatus, ippsGainQuant_G729_16s, (
       const Ipp16s *pSrcAdptTarget, const Ipp16s *pSrcFltAdptVector,
       const Ipp16s * pSrcFixedVector, const Ipp16s *pSrcFltFixedVector, 
       Ipp16s *pSrcDstEnergyErr, Ipp16s *pDstQGain, Ipp16s *pDstQGainIndex, Ipp16s tameProcess)) 
/* ===================== Decoder functios =======================  */
IPPAPI( IppStatus, ippsDecodeGain_G729_16s, (Ipp32s energy, Ipp16s *pPastEnergy,
       const Ipp16s *quaIndex, Ipp16s *pGain) )
IPPAPI( IppStatus, ippsRandomNoiseExcitation_G729B_16s, (Ipp16s *pSeed, Ipp16s *pExc, int len) )
IPPAPI( IppStatus, ippsGainControl_G729_16s_I, (const Ipp16s *pSrc, Ipp16s *pSrcDst, 
       Ipp16s *pGain) )          
IPPAPI( IppStatus, ippsGainControl_G729A_16s_I, (const Ipp16s *pSrc, Ipp16s *pSrcDst, 
       Ipp16s *pGain) )          
/* ================ Filters ==================== */
IPPAPI( IppStatus, ippsResidualFilter_G729_16s, ( const Ipp16s * pSrcSpch, const Ipp16s * pSrcLPC,
        Ipp16s * pDstResidual) )
IPPAPI( IppStatus, ippsSynthesisFilter_G729_16s,
       (const Ipp16s * pSrcResidual, const Ipp16s * pSrcLPC, Ipp16s * pSrcDstSpch))
IPPAPI (IppStatus, ippsSynthesisFilterZeroStateResponse_NR_16s, 
       (const Ipp16s * pSrcLPC, Ipp16s * pDstImp, int len, int scaleFactor))
IPPAPI(IppStatus, ippsLongTermPostFilter_G729_16s,(Ipp16s gammaFactor, int valDelay, 
       const Ipp16s *pSrcDstResidual, Ipp16s *pDstFltResidual, Ipp16s *pResultVoice ))
IPPAPI(IppStatus, ippsLongTermPostFilter_G729A_16s,(Ipp16s valDelay, const Ipp16s * pSrcSpch,
       const Ipp16s * pSrcLPC, Ipp16s * pSrcDstResidual, Ipp16s * pDstFltResidual))
IPPAPI(IppStatus, ippsLongTermPostFilter_G729B_16s,(Ipp16s valDelay, const Ipp16s * pSrcSpch,
       const Ipp16s * pSrcLPC, Ipp16s * pSrcDstResidual, Ipp16s * pDstFltResidual,
       Ipp16s * pResultVoice, Ipp16s frameType))
IPPAPI( IppStatus, ippsShortTermPostFilter_G729_16s, (const Ipp16s * pSrcLPC, 
        const Ipp16s * pSrcFltResidual, Ipp16s * pSrcDstSpch, Ipp16s * pDstImpulseResponse))
IPPAPI( IppStatus, ippsShortTermPostFilter_G729A_16s,(const Ipp16s * pSrcLPC, 
        const Ipp16s * pSrcFltResidual,Ipp16s * pSrcDstSpch))
IPPAPI (IppStatus, ippsTiltCompensation_G729_16s,(const Ipp16s * pSrcImpulseResponse,
                                        Ipp16s * pSrcDstSpch))
IPPAPI (IppStatus, ippsTiltCompensation_G729A_16s,
       (const Ipp16s * pSrcLPC,Ipp16s * pSrcDstFltResidual))
IPPAPI( IppStatus, ippsHarmonicFilter_16s_I, (Ipp16s val, int T, Ipp16s *pSrcDst, int len) )  

IPPAPI( IppStatus, ippsHighPassFilterSize_G729, (int *pSize) )
IPPAPI( IppStatus, ippsHighPassFilterInit_G729, (const Ipp16s *pCoeff, char* pMemUpdated) )
IPPAPI( IppStatus, ippsHighPassFilter_G729_16s_ISfs, (Ipp16s* pSrcDst, int len, int scaleFactor,
        char* pMemUpdated) )

IPPAPI (IppStatus, ippsIIR16sLow_G729_16s, 
        (const Ipp16s *pCoeffs, const Ipp16s *pSrc, Ipp16s *pDst, Ipp16s *pMem))
IPPAPI( IppStatus, ippsPreemphasize_G729A_16s, (Ipp16s gamma, const Ipp16s *pSrc, Ipp16s *pDst, 
       int len, Ipp16s* pMem) ) 
IPPAPI( IppStatus, ippsPreemphasize_G729A_16s_I, (Ipp16s gamma, Ipp16s *pSrcDst, int len, 
       Ipp16s* pMem) ) 

/* ========= Misc ================*/
IPPAPI( IppStatus, ippsInterpolate_G729_16s, (const Ipp16s *pSrc1, const Ipp16s *pSrc2, 
        Ipp16s *pDst, int  len) )
IPPAPI( IppStatus, ippsInterpolateC_G729_16s_Sfs, (const Ipp16s *pSrc1, Ipp16s val1,
        const Ipp16s *pSrc2, Ipp16s val2, Ipp16s *pDst, int  len, int scaleFactor) )
IPPAPI( IppStatus, ippsInterpolateC_NR_G729_16s_Sfs, (const Ipp16s *pSrc1, Ipp16s val1,
        const Ipp16s *pSrc2, Ipp16s val2, Ipp16s *pDst, int  len, int scaleFactor) )
        
#ifdef __cplusplus
}
#endif

#endif /* _IPPSC_H_ */ 

/* End of file ippSC.h */
