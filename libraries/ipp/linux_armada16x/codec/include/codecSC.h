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


#ifndef _CODECSC_H_
#define _CODECSC_H_

/* Include Intel IPP external header file(s). */
#include "codecDef.h"	/* General Codec external header file*/
#include "ippSC.h"		/* Speech Codec IPP external header file*/


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CALLBACK
#define _CALLBACK __STDCALL
#endif


/***** Data Types, Data Structures and Constants ******************************/

/***** Codec-specific Definitions *****/
/* Speech Codecs */
//***********************
// IPP GSM-AMR constants
//***********************/
#define		IPP_GSMAMR_FRAME_LEN		160		/* GSM-AMR frame length, in terms of PCM samples */
#define		IPP_GSMAMR_SUBFRAME_LEN		40		/* GSM-AMR subframe length, in terms of PCM samples */
#define		IPP_GSMAMR_NUM_SUBFRAME		4		/* GSM-AMR number of subframes contained in one frame */
#define		IPP_GSMAMR_LPC_ORDER		10		/* GSM-AMR LPC analysis order */
#define		IPP_GSMAMR_MAXLAG			143		/* GSM-AMR maximum pitch lag */

/**************************************
// IPP GSM-AMR bitstream buffer lengths
***************************************/
#define		IPP_GSMAMR_BITSTREAM_LEN_475	15	/* GSM-AMR 4.75 kbps minimum packed frame buffer byte allocation */
#define		IPP_GSMAMR_BITSTREAM_LEN_515	16	/* GSM-AMR 5.15           "             "             "          */
#define		IPP_GSMAMR_BITSTREAM_LEN_59		18	/* GSM-AMR 5.9            "	            "             "          */
#define		IPP_GSMAMR_BITSTREAM_LEN_67		20	/* GSM-AMR 6.7            "	            "             "          */   
#define		IPP_GSMAMR_BITSTREAM_LEN_74		22	/* GSM-AMR 7.4            "	            "             "          */       
#define		IPP_GSMAMR_BITSTREAM_LEN_795	23	/* GSM-AMR 7.95           "	            "             "          */           
#define		IPP_GSMAMR_BITSTREAM_LEN_102	29	/* GSM-AMR 10.2           "	            "             "          */               
#define		IPP_GSMAMR_BITSTREAM_LEN_122	34	/* GSM-AMR 12.2           "	            "             "          */     
#define		IPP_GSMAMR_BITSTREAM_LEN_DTX	8	/* SID/SID UPDATE         "	            "             "          */     
#define		IPP_GSMAMR_DECODER_TV_LEN		250	/* GSM-AMR decoder test vector length, 16-bit words (3GPP unpacked format) */

/**************************************
// AMR-WB constants
***************************************/
#define		IPP_AMRWB_FRAMELENGTH			320	/* AMR wideband frame length */


/**************************************
// 13K QCELP constants
***************************************/
#define IPP_QCELP_FRAME_LEN   160/* Overall frame size */


/***************************************
// IPP GSM-AMR enumerated TX frame types 
***************************************/
typedef enum
{	
	TX_SPEECH = 0,
	TX_SID_FIRST,
	TX_SID_UPDATE,
	TX_NO_DATA
} IppGSMAMRTXFrameType;

/***************************************
// IPP GSM-AMR enumerated RX frame types 
***************************************/
typedef enum
{
	RX_SPEECH_GOOD = 0, 
	RX_SPEECH_PROBABLY_DEGRADED, 
	RX_SPARE,
	RX_SPEECH_BAD, 
	RX_SID_FIRST, 
	RX_SID_UPDATE, 
	RX_SID_BAD, 
	RX_NO_DATA,
	RX_N_FRAMETYPES,
	RX_SPEECH_LOST
} IppGSMAMRRXFrameType;

/****************************************
// IPP GSM-AMR enumerated DTX state types 
****************************************/
typedef enum
{
	SPEECH = 0, 
	DTX, 
	DTX_MUTE
} IppGSMAMRDTXStateType;  

typedef enum
{
	IF1_FORMAT = 0,
	IF2_FORMAT,
	HEADERLESS_IF1_FORMAT,
	TV_COMPLIANCE_FORMAT
} IppGSMAMRStreamFormatId;

typedef enum
{
	AMRWB_IF1_FORMAT = 0,
	AMRWB_IF2_FORMAT,
	AMRWB_MIME_FILE_FORMAT,
	AMRWB_TV_COMPLIANCE_FORMAT
} IppAMRWBStreamFormatId;

/****************************************
// IPP G.711 enumerated law types 
****************************************/
typedef enum 
{
	IPP_ALAW = 0,
	IPP_ULAW = 1
} IppG711Law; 

/*********************************************
// IPP GSM-AMR encoder gain quantization state
*********************************************/
typedef struct
{
	/* Prediction error */							
	Ipp16s pPredQErrMR122[IPP_GSMAMR_NUM_SUBFRAME];		/* quantized prediction error, 12.2 kbps */
	Ipp16s pPredQErr[IPP_GSMAMR_NUM_SUBFRAME];			/* quantized prediction error, other rates */
	Ipp16s pPredUnQErrMR122[IPP_GSMAMR_NUM_SUBFRAME];	/* unquantized prediction error, 12.2 kbps */
	Ipp16s pPredUnQErr[IPP_GSMAMR_NUM_SUBFRAME];		/* unquantized prediction error, other rates */

	/* 4.75 kbps-specific parameters */
	Ipp16s expPredFixedGain;							/* exponent, predicted fixed codebook gain, most recent even subframe */	
	Ipp16s fracPredFixedGain;							/* mantissa, predicted fixed codebook gain, most recent even subframe */	
	Ipp16s expTargetEnergy;								/* exponent, target signal energy, most recent even subframe */	
	Ipp16s fracTargetEnergy;							/* exponent, target signal energy, most recent even subframe */	
	Ipp16s pExpEngyCoef[5];								/* exponent, energy coefficients, most recent even subframe */	
	Ipp16s pFracEngyCoef[5];							/* mantissa, energy coefficients, most recent even subframe */	
	Ipp16s adptGainSf0;									/* adaptive codebook gain, previous subframe */
	Ipp16s fixedGainSf0;								/* fixed codebook gain, most recent even subframe */
								
	/* 7.95 kbps-specific parameters */
	Ipp16s	onset;										/* onset detection flag */
	Ipp16s	prevBalanceFactor;							/* previous balance factor */
	Ipp16s	prevFixedGain;								/* previous fixed codebook gain */
	Ipp16s	pPrevCodingGain[5];							/* coding gain, previous 4 subframes */
} IppGSMAMREncGainQuantState;

/******************************************
// IPP GSM-AMR encoder DTX TX control state 
******************************************/
typedef struct
{
	Ipp16s sidUpdateCounter;							/* SID update counter */
	Ipp16s sidHandoverDebt;								/* SID hangover counter */
	Ipp16s sidUpdateRate;								/* SID update rate */
	IppGSMAMRTXFrameType prevFt;						/* previous frame type */
} IppGSMAMRTXDTXState;

/********************************************************
// IPP GSM-AMR subframe delayed encoder memory, 4.75 kbps  
********************************************************/
typedef	struct
{						
	Ipp16s	pAdaptiveTarget[IPP_GSMAMR_SUBFRAME_LEN];		/* adaptive target vector */	
	Ipp16s	pAdaptiveVector[IPP_GSMAMR_SUBFRAME_LEN];		/* adaptive codebook vector */
	Ipp16s	pAdaptiveVectorFilt[IPP_GSMAMR_SUBFRAME_LEN];	/* filtered adaptive codebook vector */
	Ipp16s	pFixedVector[IPP_GSMAMR_SUBFRAME_LEN];			/* fixed codebook vector */
	Ipp16s	pFixedVectorFilt[IPP_GSMAMR_SUBFRAME_LEN];		/* filtered fixed codebook vector */
	Ipp16s	pSynthesisFilterZiir[IPP_GSMAMR_LPC_ORDER];		/* synthesis filter IIR memory */
	Ipp16s	pitchSharpeningGain;							/* pitch sharpening filter gain (beta) */
	Ipp16s	intPitchLag;									/* integer pitch lag */
	Ipp16s	fracPitchLag;									/* fractional pitch lag */
} IppGSMAMRMemUpdateMR475;

/********************************
// IPP GSM-AMR gain decoder state   
********************************/
typedef struct 
{ 
	Ipp16s pPredQErrMR122[IPP_GSMAMR_NUM_SUBFRAME];		/* quantized prediction error, 12.2 kbps */
	Ipp16s pPredQErr[IPP_GSMAMR_NUM_SUBFRAME];			/* quantized prediction error for other bitrates */ 
	Ipp16s pPrevQAdptGain[IPP_GSMAMR_NUM_SUBFRAME+1];	/* quantized adaptive codebook gain, previous 5 subframes */
	Ipp16s pPrevQFixedGain[IPP_GSMAMR_NUM_SUBFRAME+1];	/* quantized fixed codebook gain, previous 5 subframes */
	Ipp16s prevQAdptGain;							    /* quantized adaptive codebook gain, previous frame, unsaturated */
	Ipp16s prevQFixedGain;								/* quantized fixed codebook gain, previous frame */
	Ipp16s pastQAdptGain;								/* quantized adaptive codebook gain, previous frame, saturated */
	Ipp16s pastQFixedGain;								/* quantized fixed codebook gain, previous frame */
} IppGSMAMRDecGainState;

/**************************
// IPP GSM-AMR DTX RX state   
**************************/
typedef struct
{ 
	Ipp16s pLsp[IPP_GSMAMR_LPC_ORDER];					 /* LSPs */
	Ipp16s pPrevLsp[IPP_GSMAMR_LPC_ORDER];				 /* LSPs, previous frame */
	Ipp16s pPrevQLsf[IPP_GSMAMR_LPC_ORDER];				 /* quantized LSFs, previous frame */
	Ipp16s pPrevQLsfPredResidual[IPP_GSMAMR_LPC_ORDER];	 /* quantized LSF residual, previous frame */
	Ipp16s prevLsfPointer;								 /* LSF vector update index */
	Ipp16s pPrevLsf[8*IPP_GSMAMR_LPC_ORDER];			 /* LSF history, previous 8 frames */
	Ipp16s pPrevMeanLsf[8*IPP_GSMAMR_LPC_ORDER];		 /* mean LSF history, previous 8 frames */
	Ipp16s logEnergyPointer;							 /* log energy history update index */
	Ipp16s pPrevLogEnergy[8];			                 /* log energy history, previous 8 frames */
	Ipp16s logEnergy;					                 /* log energy, current frame */
	Ipp16s logEnergyAdjust;								 /* log energy adjustment */
	Ipp16s prevLogEnergy;								 /* log-energy, previous frame */
	Ipp16s logPredMeanGain;				                 /* LSF pertubation factor */
	Ipp16s addHangOverFlag;				                 /* hangover add flag */
	Ipp16s sidFlag;						                 /* SID frame flag */
	Ipp16s hangOverVariable;			                 /* DTX hangover, fixed gain smoothing */
	Ipp16s hangOverCount;				                 /* DTX hangover counter */
	Ipp32s CNGSeed;						                 /* seed, random noise generator */
	Ipp16s sidUpdateFlag;				                 /* SID update flag */
	Ipp16s sidElapse;					                 /* frames elapsed since last SID frame */
	Ipp16s SidPeriodInv;				                 /* sidElapse reciprocal */
	Ipp16s dataUpdateFlag;				                 /* SID frame update flag */ 
	IppGSMAMRDTXStateType dtxMode;		                 /* DTX mode, current frame */
} IppGSMAMRDecDTXState;

typedef union											/* VAD1 or VAD2 state variable */
{
	IppGSMAMRVad1State vad1State;					/* VAD1 state */
	IppGSMAMRVad2State vad2State;					/* VAD2 state */
} IppGSMAMRVadState;


/***************************
// IPP GSM-AMR encoder state
***************************/
typedef struct
{
	/**********************************
	// Input speech buffer and pointers 
	**********************************/
	/* Cathy Bao: Special add for alignment */
    Ipp16s pSpeechBuf[2*IPP_GSMAMR_FRAME_LEN+4];			/* input speech buffer, 2 frames */	
	Ipp16s *pPrevSpeech;								/* previous frame in pSpeechBuf (pointer) */
	Ipp16s *pNewSpeech;									/* current frame in pSpeechBuf (pointer) */
	Ipp16s *pSpeech;									/* last subframe of previous frame in pSpeechBuf (pointer) */

	/**************
	// Preprocessor 
	**************/
	Ipp16s highpassFilterZfir[2];						/* HPF nonrecursive memory */
	Ipp32s highpassFilterZiir[2];						/* HPF recursive memory */

	/*************
	// LP analysis 
	*************/
	Ipp16s	pPrevLsp[IPP_GSMAMR_LPC_ORDER];				/* LSPs, previous frame */
	Ipp16s	pPrevLpc[IPP_GSMAMR_LPC_ORDER+1];			/* LPCs, previous frame */
	Ipp16s	pPrevQLsp[IPP_GSMAMR_LPC_ORDER];			/* LSPs, quantized, previous frame */
	Ipp16s	pPrevLsfQResidual[IPP_GSMAMR_LPC_ORDER];	/* LSF residual, quantized, previous frame */
	Ipp16s	resonanceCount;								/* LPC resonace counter */

	/************************
	// Open-loop pitch search 
	************************/
	Ipp16s	pPrevWgtSpeech[IPP_GSMAMR_MAXLAG];			/* perceptually weighted speech history */	
	Ipp16s  prevOpLag;									/* OLP lag, previous frame */
	Ipp16s  prevOpLagCount;								/* OLP lag similarity counter, previous frame */
	Ipp16s	pPastPitchLag[5];							/* Pitch lags, most recent 5 voiced half-frames */
	Ipp16s	pGainOLP[2];								/* OLP gain ([1], section 5.3, Eq. 35) */				
	Ipp16s	vValue;										/* OLPS weighting adaptation gain ([1], section 5.3, Eq. 34) */
	Ipp16s	medPastPitchLag;							/* median filtered pitch lag, most recent 5 half-frames */

	/*********************************************
	// Adaptive codebook target vector computation 
	*********************************************/
	Ipp16s	pSynthesisFilterZiir[IPP_GSMAMR_LPC_ORDER];	/* Memory of the synthesis filter, 1/Ahat(z), used
                                                           to generate shat(n) by filtering the excitation,
	                                                       u(n), i.e., u(n) -> 1/Ahat(z) -> s(n) during
                                                           target vector synthesis. 
	                                                       reference: [1], sections 5.5, 5.9. */
	Ipp16s	pSynthFiltZiirMod[IPP_GSMAMR_LPC_ORDER];    /* Memory of the synthesis filter, 1/Ahat(z), 
                                                           that is cascaded with the perceptual weighting
                                                           filter and processes the LP residual to 
                                                           generate the target, x(n), i.e.,
	                                                       resLP(n) -> 1/Ahat(z) -> A(z/gamma1)/A(z/gamma2) -> x(n). 
														   This vector contains the memory of Ahat(z),
	                                                       and it is initialized on each subframe with the last 10 samples
                                                           of the error sequence e(n)=s(n)-shat(n). 
														   reference: [1], section 5.9, p. 41 */
	Ipp16s	pWeightFiltZiirMod[IPP_GSMAMR_LPC_ORDER];   /* Recursive memory of the perceptual weighting filter
	                                                       cascaded with the synthesis filter which processes
                                                           the prediction residual in order to generate the
                                                           adaptive target, x(n), i.e.,
														   resLP(n) -> 1/Ahat(z) -> A(z/gamma1)/A(z/gamma2) -> x(n);
	                                                       This vector contains the memory of A(z/gamma1)/A(z/gamma2),
	                                                       and it is initialized on each subframe with the last 10 samples
                                                           of the sequence ew(n), i.e., the perceptually
                                                           weighted version of the error e(n)=s(n)-shat(n).
														   reference [1], section 5.9, pp. 41-42 */
	Ipp16s	pitchPrefilterGain;							/* pitch sharpening filter gain (beta) */

	/********************************************
	// Adaptive codebook closed-loop pitch search 
	********************************************/
	Ipp16s	fracPitchLag;								/* fractional pitch lag */	
	Ipp16s	prevIntPitchLag;							/* integer pitch lag, previous subframe */
	Ipp16s	pPrevQAdptGain[7];							/* adaptive codebook gain, previous subframe, quantized */

	/*******************
	// Excitation buffer
	*******************/
	Ipp16s pExcitation[IPP_GSMAMR_FRAME_LEN + IPP_GSMAMR_MAXLAG + IPP_GSMAMR_LPC_ORDER + 1];	

	/*******************
	// Gain Quantization 
	*******************/
	IppGSMAMREncGainQuantState GqState;					/* gain quantizer state */

	/***************
	// Memory update  
	***************/
	IppGSMAMRMemUpdateMR475 Update475;					/* delayed subframe parameters for 4.75 kbps 
                                                           even/odd memory update scheme  */ 
										
	/**************************
	// DTX analysis and control
	**************************/
	Ipp16s	pPrevDtxLsp[8*IPP_GSMAMR_LPC_ORDER];		/* LSP history, most recent 8 frames */
	Ipp16s	pPrevLogEnergy[8];							/* log speech energy history */
	Ipp16s	memUpdateIndex;								/* DTX buffer update index */
	Ipp16s	prevLogEngyIndex;							/* log energy index, previous frame */
	Ipp16s	pPrevLsfSubVectIndex[3];					/* LSF subvector index, previous frame */
	Ipp16s	prevLsfRefVectIndex;						/* LSF reference vector index, previous frame */
	Ipp16s	hangoverCount;								/* DTX hangover counter */
	Ipp16s	elapsedCount;								/* SID elapsed counter */
	Ipp16s  dtxEnable;									/* DTX enable: 1=on, 0=off */
	IppGSMAMRTXDTXState tx;								/* DTX TX control state */
	
	/**************************
	// VAD analysis and control 
	**************************/
	int vadModel;                                       /* selected VAD model: 1=model 1, 2=model 2 */
	Ipp16s bestCorrHpVad1;								/* VAD1 maximum normalized value of the highpass-
                                                           filtered correlation; computed by OLPS
                                                           module ([2], section 3.3.4, p. 12) */
	Ipp16s toneFlagVad1;								/* VAD1 tone flag, detects presence of information tones;
                                                           computed by OLPS module ([2], section 3.3.3, p. 11) */
	Ipp16s ltpFlagVad2;									/* VAD2 input; detects presence of strongly-tonal signals 
                                                           ([2], section 4.3.10, p. 25) when LTP gain exceeds a threshold */
	void *pVad;											/* VAD1 or VAD2 state pointer */
	IppGSMAMRVadState vadState;
} IppGSMAMREncoderState;

/***************************
// IPP GSM-AMR decoder state
***************************/
typedef struct
{
	/***************
	// LP parameters   
	***************/
	Ipp16s pPrevQLsp[IPP_GSMAMR_LPC_ORDER];				/* LSPs, previous frame */
	Ipp16s pPrevMeanLsf[IPP_GSMAMR_LPC_ORDER];			/* mean LSFs, previous frame */

	/**********************
	// Excitation generator  
	**********************/
	Ipp16s pExcitationHistory[314];		                /* excitation history buffer */	
	Ipp16s pitchSharpeningGain;					        /* pitch sharpening gain */
	Ipp16s prevIntPitchLag;				                /* pitch lag, previous */
	Ipp16s pPrevQFixedGain[7];			                /* fixed codebook gain, previous, quantized */

	/***********************************************************
	// Frame muting state machine (Error Concealment Unit - ECU) 
	***********************************************************/
	Ipp16s seed;						                /* random seed */
	Ipp16s prevBadFrameIndicator;		                /* bad frame indicator, previous frame */
	Ipp16s prevDegradedFrameIndicator;	                /* degraded frame indicator, previous frame */
	Ipp16s frameMutingStateIndex;		                /* state index, frame muting state machine */
	Ipp16s prevLTPLag;					                /* LTP lag, previous frame */
	Ipp16s pSpeechEnergyHistory[60];	                /* speech energy history, used by background noise detector */
	Ipp16s pExcitationEnergyHistory[9];	                /* excitation energy history, used by background ECU */
	Ipp16s pQAdaptiveGainHistory[9];	                /* quantized adaptive codebook gain history */
	Ipp16s inBackgroundNoise;			                /* background noise indicator; 1=present, 0=absent */
	Ipp16s voicedHangover;			 	                /* counter, time since start of voiced frame */
	Ipp16s backgroundNoiseHangover;		                /* counter, time since start of background noise */

	/**********************************************************************
	// Anti-sparseness fixed codebook processing ([1], sec. 6.1, pp. 43-44.)  
	**********************************************************************/
	Ipp16s prevQAdptGain[IPP_GSMAMR_NUM_SUBFRAME+1];	/* quantized adaptive codebook gains, 5 most recent subframes */
	Ipp16s prevQFixedGain;				                /* previous smoothed fixed codebook gain */
	Ipp16s prevImpNr;					                /* anti-sparseness adaptive modification "strength" parameter, 0,1, or 2. */
	Ipp16s lockFlag;					                /* lock flag */
	Ipp16s onset;						                /* onset flag */

	/**************
	// Gain decoder  
	**************/
	IppGSMAMRDecGainState gainState;	                /* gain decoder state */

	/*************
	// DTX decoder  
	*************/
	Ipp16s dtxHangoverCount;			                /* DTX hangover counter */
	Ipp16s elapsedCount;				                /* SID elapsed counter */
	Ipp16s prevResetFlag;				                /* previous reset flag */
	IppGSMAMRDecDTXState dtxState;		                /* DTX decoder state */

	/****************
	// Post-processer  
	****************/
	Ipp16s pAdaptPostFiltZfir[IPP_GSMAMR_LPC_ORDER];    /* adaptive postfilter non-recursive history */
	Ipp16s pAdaptPostFiltZiir[IPP_GSMAMR_LPC_ORDER];    /* adaptive postfilter recursive history */
	Ipp16s prevGainAGC;					                /* previous AGC gain */
	Ipp16s prevGainTiltComp;			                /* previous spectral tilt compensation gain */ 
	Ipp16s pHighpassZfir[2];			                /* highpass postfilter non-recursive history */
	Ipp32s pHighpassZiir[2];			                /* highpass postfilter recursive history */

	/****************
	// Speech buffer  
	****************/
	Ipp16s synSpch[IPP_GSMAMR_FRAME_LEN+IPP_GSMAMR_LPC_ORDER];	/* synthesised speech */
	int bitRate;
} IppGSMAMRDecoderState;

typedef struct {
	IppGSMAMRStreamFormatId formatInd;	
									
	int frameType;				/* Encoder's output, decoder's input, only used for 
								// formatInd = 2 */
	IppSpchBitRate bitRate;		/*  Input for both encoder and decoder */

} IppGSMAMRFormatConfig;

typedef struct {
	IppAMRWBStreamFormatId bitstreamFormatId; 
									
	IppSpchBitRate bitRate;		/*  Input for the encoder */

} IppAMRWBCodecConfig;

/********************
 G.723.1/A constants  
 ********************/
#define IPP_G723_FRAME_LEN			240		/* G.723.1/A frame length, in terms of PCM samples */
#define IPP_G723_SUBFRAME_LEN		60		/* G.723.1/A subframe length, in terms of PCM samples */
#define IPP_G723_LPC_ORDER			10		/* G.723.1/A LPC analysis order */
#define IPP_G723_LPCWIN_LEN			180		/* G.723.1/A LPC analysis Hamming window length */	
#define IPP_G723_NUM_SUBFRAME		4		/* G.723.1/A number of subframes contained in one frame */
#define IPP_G723_MAXLAG				145		/* G.723.1/A longest possible pitch lag (55 Hz) */
#define IPP_G723_TAMING_PARAMS		5		/* G.723.1/A error taming parameter vector length */
#define IPP_G723_COVMATDIM			416		/* G.723.1/A size of Toepliz covariance matrix for ACELP CB search */
#define IPP_G723_GSU_INIT			0x1000	/* G.723.1/A decoder gain scaling unit history initialization constant */
#define	IPP_G723_HPF_ENABLE		    1		/* G.723.1/A encoder highpass filter enable (on) */
#define	IPP_G723_HPF_DISABLE		0		/* G.723.1/A encoder highpass filter disable (off) */
#define	IPP_G723_POSTFILT_ENABLE	1		/* G.723.1/A decoder postfilter enable (on) */
#define	IPP_G723_POSTFILT_DISABLE   0		/* G.723.1/A decoder postfilter disable (off) */
#define	IPP_G723_ERASURE_FRAME		1		/* G.723.1/A decoder erasure frame detected (invalid frame) */
#define	IPP_G723_NON_ERASURE_FRAME  0		/* G.723.1/A decoder non-erasure frame (valid frame) */
#define IPP_G723_FRAMETYPE_NONTX    0       /* G.723.1/A non-transmitted silence frame (nonTX SID) */
#define IPP_G723_FRAMETYPE_VOICE    1		/* G.723.1/A active speech frame (5.3 or 6.3 kbps) */   
#define IPP_G723_FRAMETYPE_SID      2		/* G.723.1/A SID (silence interval description) frame */   

//***************************************
// IPP G723.1/A bitstream buffer lengths
//***************************************
#define	IPP_G723_BITSTREAM_LEN_5300	20		/* G.723.1/A 5.3 kbps minimum packed frame buffer byte allocation */
#define	IPP_G723_BITSTREAM_LEN_6300	24		/* G.723.1/A 5.3 kbps minimum packed frame buffer byte allocation */

/************************
 G.723.1 encoder state   
 ************************/
typedef struct
{
	Ipp16s	highpassFilterZfir;												/* preprocesser HPF nonrecursive memory */
	Ipp32s	highpassFilterZiir;												/* preprocesser HPF recursive memory */
	Ipp16s	prevSpch[2*IPP_G723_SUBFRAME_LEN];								/* half-frame (2 subframes) speech history buffer for LP analysis */
	Ipp16s	sineDtct;														/* sinusoidal input detection parameter */
	Ipp16s	prevLsf[IPP_G723_LPC_ORDER];									/* previous frame LSP history buffer */
	Ipp16s	perceptualWeightFilterZfir[IPP_G723_LPC_ORDER];					/* perceptual weighting filter nonrecursive memory */
	Ipp16s	perceptualWeightFilterZiir[IPP_G723_LPC_ORDER];					/* perceptual weighting filter recursive memory */
	Ipp16s	prevWgtSpch[IPP_G723_MAXLAG];									/* perceptually weighted speech history buffer for OLPS */
	Ipp16s	combinedFilterZfir[IPP_G723_LPC_ORDER];							/* combined filter (LPC synthesis+HNS+PWF) nonrecursive memory */
	Ipp16s	combinedFilterZiir[IPP_G723_MAXLAG];							/* combined filter (LPC synthesis+HNS+PWF) recursive memory */
	Ipp16s	prevExcitation[IPP_G723_MAXLAG];								/* previous frame excitation sequence history */
	Ipp32s	errorTamingParams[IPP_G723_TAMING_PARAMS];						/* error "taming" parameters for adaptive codebook search constraint */
	Ipp16s	openLoopPitchLag[IPP_G723_NUM_SUBFRAME];						/* VAD OL pitch lag history for voicing classification */ 
	Ipp16s	vadLpc[IPP_G723_LPC_ORDER];										/* VAD LPC inverse filter coefficients */
	Ipp16s	randomSeedCNG;													/* CNG excitation generator random seed */			
	Ipp16s	prevDTXFrameType;												/* DTX frame type history */
	Ipp16s	sidGain;														/* quantized CNG excitation gain (G~sid, [3], p. 9) */
	Ipp16s	targetExcitationGain;											/* CNG target excitation gain (G~t, [3], p. 10) */
	Ipp16s	frameAutoCorr[(IPP_G723_LPC_ORDER+1)*IPP_G723_NUM_SUBFRAME];	/* VAD frame (summation) autocorrelation histories */
	Ipp16s	frameAutoCorrExp[IPP_G723_NUM_SUBFRAME];						/* VAD frame (summation) autocorrelation exponent histories */
	Ipp16s	sidLsf[IPP_G723_LPC_ORDER];										/* CNG SID LSF vector (p~t associated with Asid(z)) */
	IppG723AVadState vadState;												/* VAD state */
	IppG723ADtxState dtxState;												/* DTX state */
} IppG723EncoderState; 

/************************
 G.723.1 decoder state   
 ************************/
typedef struct
{
	Ipp16s	consecutiveFrameErasures;							/* consective frame erasure count */
	Ipp16s	prevLsf[IPP_G723_LPC_ORDER];						/* previous frame LSP history buffer */
	Ipp16s	interpolationGain;									/* interpolation frame excitation gain */
	Ipp16s	interpolationIndex;									/* interpolation frame adaptive codebook index */
	Ipp16s	prevExcitation[IPP_G723_MAXLAG];					/* previous frame excitation sequence history */
	Ipp16s	randomSeed;											/* random seed for interplation frame unvoiced excitation synthesis */ 
	Ipp16s	synthesisFilterZiir[IPP_G723_LPC_ORDER];			/* speech synthesis filter recursive memory */
	Ipp16s	formantPostfilterZfir[IPP_G723_LPC_ORDER];			/* formant postfilter nonrecursive memory */
	Ipp16s	formantPostfilterZiir[IPP_G723_LPC_ORDER];			/* formant postfilter recursive memory */
	Ipp16s	autoCorr;											/* formant postfilter correlation coefficient (k) */
	Ipp16s	prevGain;											/* gain scaling unit gain history (previous frame gain) */
	Ipp16s	randomSeedCNG;										/* CNG excitation generator random seed */			
	Ipp16s	prevDTXFrameType;									/* DTX frame type history */
	Ipp16s	sidGain;											/* quantized CNG excitation gain (G~sid, [3], p. 9) */
	Ipp16s	targetExcitationGain;								/* CNG target excitation gain (G~t, [3], p. 10) */
	Ipp16s	sidLsf[IPP_G723_LPC_ORDER];							/* CNG SID LSF vector (p~t associated with Asid(z)) */
	IppSpchBitRate bitRate;										/* bitrate history */
	Ipp16s  tst[240];
} IppG723DecoderState;


/************************
 13K QCELP enum types
 ************************/
/* Rate Reduce mode for QCELP */
typedef enum {
    IPP_QCELP_RRM_DISABLE   = 0,
    IPP_QCELP_RRM_LEVEL1    = 1,
    IPP_QCELP_RRM_LEVEL2    = 2,
    IPP_QCELP_RRM_LEVEL3    = 3,
    IPP_QCELP_RRM_LEVEL4    = 4
} IppQCELPRateReduceMode;

/* encoded rate */
typedef enum {
    IPP_QCELP_BLANK			= 0,
    IPP_QCELP_EIGHTH		= 1,
    IPP_QCELP_QUARTER		= 2,
    IPP_QCELP_HALF			= 3,
    IPP_QCELP_FULL			= 4,   
    IPP_QCELP_ERASURE		= 0xe
} IppQCELPFrameType;

typedef enum {
    IPP_QCELP_CHANGE_RRM = 0,
} IpppQCELPCommand;



/***** Multimedia Codec Functions *************************************************/

/***** Speech Codecs *****/

/* GSM-AMR Codec */

/* Encoder */
IPPCODECAPI( IppCodecStatus, EncoderInitAlloc_GSMAMR, (
	void **ppDs, 
	int dtxEnable, 
	int vadModelSelect, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI(IppCodecStatus, EncoderFree_GSMAMR, (
	void **ppDs, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI( IppCodecStatus, Encode_GSMAMR, (
	IppSound *pSrcSpeech, 
	IppBitstream *pDstBitStream, 
	void *pGSMAMREncoderState, 
	IppGSMAMRFormatConfig *pFormatConfig
)) 

/* Decoder */
IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_GSMAMR, (
	void	**ppDs, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI(IppCodecStatus, DecoderFree_GSMAMR, (
	void **ppDs, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI( IppCodecStatus, Decode_GSMAMR, (
	IppBitstream *pSrcBitstream, 
	IppSound *pDstSpeech, 
	void *pDecoderState, 
	IppGSMAMRFormatConfig *pFormatConfig
))

/* G.723.1 Codec */
IPPCODECAPI( IppCodecStatus,  EncoderInitAlloc_G723, (
	void **ppDs, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI(IppCodecStatus, EncoderFree_G723,(
	void **ppDs, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI( IppCodecStatus, Encode_G723, (
	IppSound *pSrcSpeech,
	IppBitstream *pDstBitstream, 
	IppSpchBitRate bitRate, 
	int enableVad, 
	int enableHighpassFilter,
	void *pEncoderState
))

IPPCODECAPI( IppCodecStatus,	DecoderInitAlloc_G723, (
	void **ppDs, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI(IppCodecStatus, DecoderFree_G723, (
	void **ppDs, 
	MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI( IppCodecStatus, Decode_G723, (
	IppBitstream *pSrcBitstream, 
	IppSound *pDstSpeech, 
	Ipp16s erasureFrame, 
	int enablePostFilter,
	void *pDecoderState
))

/* AMR-WB codec */

/* Encoder functions for AMR-WB */
IPPCODECAPI(IppCodecStatus, EncoderInitAlloc_AMRWB,(
			void **ppDs, int dtxEnable, MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI(IppCodecStatus, Encode_AMRWB, (
	IppSound *pSrcSpeech, IppBitstream *pDstBitStream, 
	IppAMRWBCodecConfig  *pFormatConfig, void *pEncoderState))

IPPCODECAPI(IppCodecStatus, EncoderFree_AMRWB, (
			void **ppDs, MiscGeneralCallbackTable* pCallBackTable
))

/* Decoder functions for AMR_WB */
IPPCODECAPI(IppCodecStatus, DecoderInitAlloc_AMRWB, (
			void	**ppDs, MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI(IppCodecStatus, DecoderFree_AMRWB, (
			void **ppDs, MiscGeneralCallbackTable* pCallBackTable
))

IPPCODECAPI (IppCodecStatus,  Decode_AMRWB, (
				IppBitstream		 *pSrcBitstream, 
				IppSound			 *pDstSpeech, 
				void				 *pDecoderState, 
				IppAMRWBCodecConfig  *pDecoderConfig
				))

/* ==================== G.729 Codec definitions ================= */
/* G729 has three flavors.  Note that annex A is a requirement for annex B (VAD). */
typedef	enum {
	G729, G729A, G729AB
} IppG729CodecType;
/* ==================== G.729 Codec API ================= */
IPPCODECAPI (IppCodecStatus, EncoderInitAlloc_G729, (void **ppDst, 
										   IppG729CodecType codecType,
										   MiscGeneralCallbackTable* pCallBackTable))
IPPCODECAPI (IppCodecStatus, EncoderFree_G729, (void **ppDst, 
											MiscGeneralCallbackTable* pCallBackTable))
IPPCODECAPI (IppCodecStatus, Encode_G729, (void* pEncoderState, 
                             const Ipp16s *pSrcSpeech, Ipp16s *pDstBitstream, IppG729CodecType codecType))

IPPCODECAPI (IppCodecStatus, DecoderInitAlloc_G729, (void **ppDst, 
											IppG729CodecType codecType,
											MiscGeneralCallbackTable* pCallBackTable))
IPPCODECAPI (IppCodecStatus, DecoderFree_G729, (void **ppDst,
											MiscGeneralCallbackTable* pCallBackTable))
IPPCODECAPI (IppCodecStatus, Decode_G729, (void* pSrcDecoderState,
                                 const Ipp16s *pSrcBitstream, Ipp16s *pDstSpeech, IppG729CodecType codecType))



/* Encoder functions for QCELP */

IPPCODECAPI(IppCodecStatus, EncoderInitAlloc_QCELP,(
	void **ppDs, MiscGeneralCallbackTable* pCallBackTable))

IPPCODECAPI(IppCodecStatus, Encode_QCELP, (
	IppSound *pSrcSpeech, IppBitstream *pDstBitStream, 
	IppQCELPFrameType *mode, void *pEncoderState))

IPPCODECAPI(IppCodecStatus, EncoderFree_QCELP, (
	void **ppDs, MiscGeneralCallbackTable* pCallBackTable))

IPPCODECAPI(IppCodecStatus, EncodeSendCmd_QCELP, (
            int cmd, void *pInParam, void *pEncoderState))

// Decoder functions for QCELP 
IPPCODECAPI(IppCodecStatus, DecoderInitAlloc_QCELP, (
	void **ppDs, MiscGeneralCallbackTable* pCallBackTable))

IPPCODECAPI(IppCodecStatus, DecoderFree_QCELP, (
	void **ppDs, MiscGeneralCallbackTable* pCallBackTable))

IPPCODECAPI (IppCodecStatus,  Decode_QCELP, (
	IppBitstream *pSrcBitstream, IppSound *pDstSpeech,
	int mode, void *pDecoderState))
    
/*============================ G.711 Codec API ====================================*/
IPPCODECAPI(IppCodecStatus, Encode_G711, (const Ipp16s *pSrc, int length, IppG711Law lawused, Ipp8u *pDst))

IPPCODECAPI(IppCodecStatus, Decode_G711, (const Ipp8u *pSrc, int length, IppG711Law lawused, Ipp16s *pDst))

IPPCODECAPI(IppCodecStatus, ALawToULaw_G711, (const Ipp8u *pSrc, int length, Ipp8u *pDst))

IPPCODECAPI(IppCodecStatus, ULawToALaw_G711, (const Ipp8u *pSrc, int length, Ipp8u *pDst))


#ifdef __cplusplus
}
#endif

#endif    /* #ifndef _CODECSC_H_ */

/* EOF */


