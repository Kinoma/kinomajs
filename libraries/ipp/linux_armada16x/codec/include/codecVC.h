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


#ifndef _CODECVC_H_
#define _CODECVC_H_

/* Include Intel IPP external header file(s). */
#include "codecDef.h"	/* General Codec external header file*/
#include "ippVC.h"		/* Video Codec IPP external header file*/



#ifdef __cplusplus
extern "C" {
#endif

#define MOTION_SLOW	0
#define MOTION_MEDIUM	1
#define MOTION_HIGH	2
/***** Data Types, Data Structures and Constants ******************************/

typedef enum _IppCommand{
	/* Command for Encoder */
	IPPVC_INIT_HWME			 = 0,	/* HWME Init */
	IPPVC_RUN_HWME			 = 1,	/* HWME Run  */
	IPPVC_CONTROL_HWME		 = 2,	/* HWME Control */
	IPPVC_CLOSE_HWME		 = 3, 	/* HWME Close */
	IPPVC_REFRESH_INTRAFRAME = 4, 	/* Refresh Intra Frame encoding */
	IPPVC_SET_IVOPQUANTSTEP	 = 5, 	/* Set IVOP Quant Step flag */
	IPPVC_GET_IVOPQUANTSTEP	 = 6, 	/* Get IVOP Quant Step flag */
	IPPVC_SET_PVOPQUANTSTEP	 = 7, 	/* Set PVOP Quant Step flag */
	IPPVC_GET_PVOPQUANTSTEP	 = 8, 	/* Get PVOP Quant Step flag */
	IPPVC_SET_MESTRATEGY	 = 9, 	/* Set ME Strategy flag */
	IPPVC_GET_MESTRATEGY	 = 10, 	/* Get ME Strategy flag */
	IPPVC_SET_MEFLOWCONTROL	 = 11, 	/* Set ME Flow Control flag */
	IPPVC_GET_MEFLOWCONTROL	 = 12, 	/* Get ME Flow Control flag */
	IPPVC_SET_FREEZEFRAME    = 13, 	/* Set Encoding one Freeze Frame flag */
	IPPVC_GET_PICTURETYPE    = 14, 	/* Get Current Picture Type */
    IPPVC_SET_FRAMERATE      = 15,  /* Change frame rate */
    IPPVC_SET_UPDATEENCPARA  = 16,  /* update encoder parameters*/
    // for encoder AIR/CIR
	IPPVC_SETAIRCIRSTRATEGY  = 17,	/* Set AIR/CIR strategy.  If strategy==-1, no AIR/CIR. If it == 0, use internal AIR/CIR mechanism. If it==1, use user-defined mechanism. */
	//following command only meaningful for AIR/CIR strategy==0
	IPPVC_SETAIRENABLE       = 18,  /* Enable/Disable AIR */
	IPPVC_SETCIRENABLE       = 19,  /* Enable/Disable CIR */
	IPPVC_GETAIRSAD00TH      = 20,  /* Get AIR threshold for sad at MV(0,0) */
	IPPVC_SETAIRSAD00TH      = 21,  /* Set AIR threshold for sad at MV(0,0) */
	IPPVC_GETAIRSAD00SUM     = 22,  /* Get the sum of sad at MV(0,0) for last P-VOP only when AIR is enabled */
	IPPVC_SETAIRMBINTRACODEDPERFRAME    = 23,  /* Set the number of MB(with high activity) to be intra coded according to CIR rule per frame */
	IPPVC_SETAIRREFRESHTIME  = 24,  /* Set the number of times a MB should be intra coded to be considered refreshed according to AIR rule */
	IPPVC_SETCIRMBINTRACODEDPERFRAME    = 25,  /* Set the number of MB to be intra coded according to CIR rule per frame */
    IPPVC_SET_FRAMESKIPDISABLE = 26,
	IPPVC_SET_MAXIBITRATES = 27, /* Set the maximum bitrate constrain in rate control */

	/* Command for Decoder */
	IPPVC_SET_NSCCHECKDISABLE	= 100, 	/* Disable NSC Check flag */
	IPPVC_SEEK_NEXTSYNCCODE		= 101, 	/* Perform seeking for next sync code */
	IPPVC_GET_NEXTPICTURETYPE	= 102, 	/* Perform seeking for next sync code */
	IPPVC_SET_OUTPUTDELAYDISABLE= 103, 	/* Disalbe output frame delay and force decoder to output one frame every time */

    IPPVC_GET_PICSIZE           = 106, /* get the pic size of the new sequence*/

    IPPVC_SET_SKIPMODE			= 107,  /* set the skip mode */

	IPPVC_GET_DPBSIZE			= 108,	/* get the spec defined DPB size */
	IPPVC_GET_POCTYPE  			= 109,	/* get the POC type */
	IPPVC_SEEK_LASTSLICEENDPOS = 110,	/* For MPEG2 MP decoder only, seek the end position of the last slice of current picture in input bitstream*/
	IPPVC_SET_DECODENEWPIC = 111,		/* For MPEG2 MP decoder only, set the codec state to ready for decoding a new picture*/
	IPPVC_SET_DECODENEWFRAME = 112,		/* For MPEG2 MP decoder only, set the codec state to ready for decoding a new frame, a frame could be a frame picture or a pair of field picture*/
    /* command for vmeta decoder*/
    IPPVC_STOP_DECODE_STREAM        = 200, /*send command to stop decoding stream*/
    IPPVC_END_OF_STREAM             = 201, /*inform vmeta decoder no stream data*/
    IPPVC_REINIT_DECODER            = 202, /*reinitialize vmeta decoder*/
    IPPVC_SET_INTWAITEVENTDISABLE   = 203, /*disable or enable internally waiting interrupt*/
    IPPVC_FLUSH_BUFFER              = 204, /*flush pushed input or output buffers*/
    
	/* Command for Decoder */
	IPPVC_ENABLE_IS			 = 1000,	/* Enable image stabilizer */
	IPPVC_DISABLE_IS		 = 1001,	/* Disable image stabilizer */
	/* for h.264 decoder buffer sharing */
	IPPVC_SET_FRAMEMEMOP     = 1002,    /* for h.264 decoder buffer sharing only, set BMM frame buffer call back functions */
	IPPVC_MORE_FRAMEBUFFUER  = 1003,	/* for h.264 decoder buffer sharing only, add one more BMM frame buffer to decoder buffer list */
} IppCommand;

typedef enum _IppSkipMode {
    IPPVC_SKIPMODE_0         = 0,    /* Normal decoding */
    IPPVC_SKIPMODE_1		 = 10,   /* Fast deblocking for all frames */
    IPPVC_SKIPMODE_2		 = 20,	 /* No deblocking for all frames */
    //IPPVC_SKIPMODE_2_1		 = 21,
    //IPPVC_SKIPMODE_2_2		 = 22,
    //IPPVC_SKIPMODE_2_3		 = 23,
    IPPVC_SKIPMODE_2_5		 = 25,	 /* Skip 1/2 non-reference frames */
    IPPVC_SKIPMODE_3		 = 30,	 /* Skip all non-reference frames */

} IppSkipMode;

typedef enum _IppQuantMode {
	Q_H263		=	0,
	Q_MPEG4		=	1
} IppQuantMode;

typedef enum _IppRCType {
	RC_MPEG4		= 0,
	RC_TMN8			= 1,
	RC_NEWTONRAPSON		= 2,
	RC_VT				= 3,
	RC_NULL			= 255

} IppRCType;

/* Macroblock Types */
typedef enum {
	IPP_VIDEO_FORBIDDEN		= 0,	/* forbidden */
	IPP_VIDEO_SUBQCIF		= 1,	/* sub-QCIF */
	IPP_VIDEO_QCIF			= 2,	/* QCIF */
	IPP_VIDEO_CIF			= 3,	/* CIF */
	IPP_VIDEO_4CIF			= 4,	/* QCIF */
	IPP_VIDEO_16CIF			= 5,	/* CIF */
	IPP_VIDEO_CUSTOM		= 6,	/* Custom Format */
	IPP_VIDEO_EXT_PTYPE		= 7		/* extended PTYPE */
} IppSourceFormat;

/***** Codec-specific Definitions *****/
/*** Video Codecs ***/
	
/** H.263 and Mpeg4 Share **/

typedef struct _IppPosition
{
   Ipp8u *ptr;           /* point to the start byte address */
   int   bitoff;         /* bit offset in this byte */
} IppPosition;

typedef struct _IppPtrSet
{
    unsigned char *pYPtr;	/* pointer to the start address of luminance (Y) plane */
    unsigned char *pCbPtr;	/* pointer to the start address of chrominance (Cb) plane */
    unsigned char *pCrPtr;	/* pointer to the start address of chrominance (Cr) plane */
    unsigned char *pBYPtr;	/* pointer to the start address of Binary plane */
    unsigned char *pAPtr;	/* pointer to the start address of Alpha plane */
    unsigned char *pBYModPtr;	/* pointer to the start address of BABMode plane */
} IppPtrSet;

typedef enum _IppMEStrategy {
	ME_PERFORMANCE_BIAS		= 0,
	ME_QUALITY_BIAS			= 1
} IppMEStrategy;

typedef struct _IppMEFlowContrl
{ 
    int bBlockSearchDisabled;
    int iEarlyExitThresForZMD;
    int iFavorIntra;
    int iEarlyExitThresInt16;
	int iEarlyExitThresInt8;
	int iEarlyExitThresHalf16;
} IppMEFlowContrl;


typedef struct _IppHWMEDynamicContrl
{ 
    int iOverflowSADVal;
    int iPreferZeroMVFactor;
    int bPreferZeroMVEnable;
    int iHalfStepMode;
	int bHalfStepEnable;
	int bBlockSearchEnable;
	int iEarlyExitThresSAD;
} IppHWMEDynamicContrl;


/* define image stabilizer callback function types */
typedef IppCodecStatus (__STDCALL *IppISCallbackInitAlloc)(void* , void** );
typedef IppCodecStatus (__STDCALL *IppISCallbackFree)(void** ppState);
//typedef (*appiControl_IS)			(void* pConfig, void** ppState);
typedef IppCodecStatus (__STDCALL *IppISCallbackPreprocess)(IppPicture *,
															IppMotionVector* ,
															IppMotionVector* ,
															int , void *);
typedef IppCodecStatus (__STDCALL *IppISCallbackPostprocess)(IppPicture *,
															 IppMotionVector* ,
															 Ipp32s* , void* );


/* Input structure for image stabilizer enable command */
typedef struct _IppISDynamicContrl {

	void						*pISConfig;
	IppISCallbackInitAlloc		fISInitAlloc;
	IppISCallbackFree			fISFree;
	IppISCallbackPreprocess		fISPreprocess;
	IppISCallbackPostprocess	fISPostprocess;
} IppISDynamicContrl;


/* Image stabilizer's control structure */
typedef struct _IppISConfig {

	int					iVolWidth;
	int					iVolHeight;
	int					iBorderExtension;	/* width of VOL's neighbor pixels */
	int					iInterpolation;		/* interpolation method for jitter correction 1 - bilinear */
	int					iMEAlgorithm;		/* search algorithm for jitter estimation; 0 - SEA, 1 - MVFAST */
	int					iSearchRes;			/* search resolution: 0 - Int, 1 - Half */
	int					iMaxKBNum;			/* max key block number */
	Ipp32s				iNoiseSAD;			/* estimation of noise SAD in 16x16 block */
	int					iMaxJitter;			/* Max jitter or jolt */
	int					iPanningVar;		/* the variation of panning */
	int					iTrackErrThreshold;	/* Threshold of tracking error */
	Ipp32s				iTrackSpeed;		/* coefficient for the track speed */
	/* estimation result */
	IppMotionVector*	pJMV;	/* Jitter MV */
	IppMotionVector*	pGMV;	/* Global MV */
	/* Callback functions */
	MiscCallocCallback	fMemAlloc;
    MiscFreeCallback	fMemFree;
} IppISConfig;

//for AIR/CIR
typedef struct _IppAIRCIRControl {
	int		iAIRCIRStrategy;	/* If it==-1, no AIR/CIR. If it == 0, use internal AIR/CIR mechanism. If it==1, use user-defined mechanism */
	int		(*pfunAIRCIRRefresh)(void * pAIRCIRObj, int curMBIdx, int SAD00, int bIntra_MEdecided);
	void*	pAIRCIRObj;
	
	//below only meaningful for iAIRCIRStrategy==0
	/* Adapative Intra Macroblock Refreshment Enable. 0--disable, 1--enable */
	int  bAIREnable;
	/* the number of MB to be intra coded according to AIR rule per frame, it should >= 0 */
	int  iAIRMBIntraCodedPerFrame;	//6 is a typical value for QCIF
	/*the number of times a MB should be intra coded to be considered refreshed according to AIR rule. Once a MB was regarded as high activity MB, it should be intra coded for nAIRRefreshTime times. It should >= 0 and <=65535 */
	Ipp16u nAIRRefreshTime;			//1 is a typical value
	/*the threshold of SAD with MV(0,0) for judging whether MB is high activity MB*/
	int  iAIRSAD00th_InitVal;	//the initial value for threshold of sad at MV(0,0). AIR mechanism decide whether the MB is high activity based on this value. This threshold is automatically updated according to average sad P-VOP by P-VOP. It should >= 0. If it is 0, all MB is considered as high activity MB.
	/*Cyclic Intra Refreshment Enable. 0--disable, 1--enable*/
	int  bCIREnable;
	/* the number of MB to be intra coded according to CIR rule per frame, it should >= 0 */
	int  iCIRMBIntraCodedPerFrame;	//4 is a typical value for QCIF

} IppAIRCIRControl;


/* info which par file provides. */
typedef struct _IppParSet {

   int bAlgorithmFromCustomer; /* Algorithm configured by customer or not */

   /* 1st Level Parameter */
   char InputRawDataFileName[100]; /* input raw data file name */
   char OutputCompressedFileName[100]; /* output file name */
   int iVolWidth;		/* VOL width */
   int iVolHeight;		/* VOL height */
   int iSrcFrmRate;		/* Frame rate */
   int iSrcSamRate;		/* sample rate */
   int iColorFormat;	/* Color Format */
   int bRCEnable;		/* Rate control enable or not */
   int iRCBitRate;		/* Target bit rate, bps as unit */
   int bHWMEEnable;		/* Enable HWME or not */    

   /* 2nd Level Parameter */
   int bObjectLayerIdentifier; /* Flag to decide whether version identification and priority is specified or not */
   int iVolVerID;		/* VOL version identification */
   int iVolPriority;	/* 3-bit code which specifies the priority of the video object layer */
   int bVolRandomAcess; /* The flag sets whether every VOP in this VOL is individually decodable */
   int iVideoObjectTypeID;	/* Video Object type identification */
   
   int iIntraDcVlcThr;	/* Intra DC VLC threshold */   
   int bRoundingCtrlEnable; /* Rounding Control enable */   
   int iRoundingCtrlInit;	/* Initial Rounding Control value */   
   int iPBetweenI;			/* Number of P-Frames between two nearest I-Frames */   
   int iClockRate;			/* Clock Rate */   
   int iNumBitsTimeIncr;	/* Number of Bits for Time Increment */
   int bComplexEstDisable;	/* Complex Estimation Disable */
   
   Ipp8u iIVOPQuantStep;	/* Quantisation parameter in I-VOP */
   Ipp8u iPVOPQuantStep;	/* Quantisation parameter in P-VOP */
   int bQuantType;		/* Quantisation method */      
   int bIntraQMatrixDefault;/* Use Default Intra Quantization Matrix */
   int bInterQMatrixDefault;/* Use Default Inter Quantization Matrix */
   Ipp8u *IntraQMatrix;	/* Luminance/Chrominance Intra Quantisation Parameter Matrix */
   Ipp8u *InterQMatrix;	/* Luminance/Chrominance Inter Quantisation Parameter Matrix */
   
   int bResyncDisable;		/* Error resillience Resync mode disable or not */
   int bDataPartitioned;	/* Error resillience Data Partition mode enable or not */
   int bReverseVLC;		/* Error resillience Reversible Varied Length Coding enable or not */
   int iVideoPacketLength;	/* Video packet length, bit unit */ 
   /*below 2 AIR parameters are supersession, 2008.9 */
   int bAdapativeIntraRefreshEnable;/* Adapative Intra Macroblock Refreshment Enable */
   int iAdapativeIntraRefreshRate;	/* the number of VOPs in which all macroblocks refreshed a round */

   int iPictureComplexity;	/* Complexity of target clips 0 - Low, 1 - Median, 2 - High */
   

   /* 3rd Level Parameter */
   int iRCType;			/* Rate control type */
   int iSearchRange;		/* Search Range in motion estimation */  
   int bUseSrcME;			/* raw data direct used in motion estimation */
   int iMEAlgorithm;		/* Motion estimation algorithm */    
   int bVolControlPara;	    /* VOL control parameters included or not */	
   int bVbvParaEnable;		/* VBV parameters included or not */
   int iVbvBitRate;			/* Peak Bit Rate */
   int iVbvBufferSize;		/* VBV Buffer Size factor */	
   int iVbvOccupancy;		/* VBV Occupancy factor */	
   int iMotionActivity;
   int iSlopeDelta;
   int iInflexionDelta;
   int iIVOPQPDelta;
   int iMaxQP;
   int iMinQP;
   int iDelayMaxFactor;
   int iDelayMinFactor;
   int iModelK;
   int iModelC;
   IppMEStrategy iMEStrategy;
   IppMEFlowContrl MEFlowContrl;
   IppHWMEDynamicContrl HWMEDynamicContrl;

   /* user defined paramter */
   void* pStreamHandler;
} IppParSet;

typedef struct _IppH263ParSet {

   int bAlgorithmFromCustomer; /* Algorithm configured by customer or not */

   /* 1st Level Parameter */
   char InputRawDataFileName[100]; /* input raw data file name */
   char OutputCompressedFileName[100]; /* output file name */
   int iSourceFormat;	/* Support SubQCIF, QCIF, CIF, 4CIF, and 16CIF*/
   int iPicWidth;
   int iPicHeight;
   int iSrcFrmRate;		/* Frame rate */   
   int iSrcSamRate;		/* sample rate */  
   int iColorFormat;		/* Color Format */      
   int bRCEnable;		/* Rate control enable or not */
   int iRCBitRate;		/* Target bit rate, bps as unit */
   int bHWMEEnable;		/* Enable HWME or not */    

   /* 2nd Level Parameter */
   
   int bRoundingCtrlEnable; /* Rounding Control enable */   
   int GOBHeaderPresent;	/* GOB Header Enable */
   int iPBetweenI;			/* Number of P-Frames between two nearest I-Frames */   
   int iIntraQuant;			/* Quantisation parameter in Intra coded picture */
   int iInterQuant;			/* Quantisation parameter in Inter coded picture */

   int optUMV;				/* flag of optional Unrestricted Motion Vector mode*/
   int optSAC;				/* flag of optional Syntax-based Arithmetic Coding mode */
   int optAP;				/* flag of optional Advanced Prediction mode */
   int optPBframes;			/* flag of optional PBframes mode */

   int optAIC;			/* flag of optional Advanced INTRA Coding mode */
   int optDF;			/* flag of optional Deblock Filtering mode */
   int optSS;			/* flag of optional Slice Structured mode */
   int BitsPerSlice;	/* Threshold of bits per slice when in SS mode */
   int optRPS;			/* flag of optional Reference Picture Selection mode */
   int optISD;			/* flag of optional Independent Segment Decoding mode */
   int optAIV;			/* flag of optional mode */
   int optMQ;			/* flag of Modified Quantization mode */

   /* MPPTYPE */
   int optRPR;			/* flag of optional Referenced Picture Resampling mode */
   int optRRU;			/* flag of optional Reduced-Resolution Update mode */

   /* 3rd Level Parameter */
   int iRCType;				/* Rate control type */
   int iSearchRange;		/* Search Range in motion estimation */  
   int iVbvBitRate;			/* Peak Bit Rate */
   int iVbvBufferSize;		/* VBV Buffer Size factor */	
   int iVbvOccupancy;		/* VBV Occupancy factor */	
   int iMotionActivity;
   int iSlopeDelta;
   int iInflexionDelta;
   int iIPictureQPDelta;
   int iMaxQP;
   int iMinQP;
   int iDelayMaxFactor;
   int iDelayMinFactor;
   int iModelK;
   int iModelC;   

// int bUseSrcME;			/* raw data direct used in motion estimation */
   int iMEAlgorithm;		/* Motion estimation algorithm */    
   IppMEStrategy iMEStrategy;
   IppMEFlowContrl MEFlowContrl;
   IppHWMEDynamicContrl HWMEDynamicContrl;

   /* user defined paramter */
   void* pStreamHandler;
} IppH263ParSet;

/** H.264 **/
/* profile defined by standard */
typedef enum {
	H264_BASELINE_PROFILE           	= 66,
	H264_MAIN_PROFILE               	= 77,
	H264_EXTENDED_PROFILE           	= 88
} H264ProfileIdc;

/* NAL unit type defined by standard */
typedef enum {
    H264_SLICE_NALU                 	= 1,
    H264_DPA_SLICE_NALU             	= 2,
    H264_DPB_SLICE_NALU             	= 3,
    H264_DPC_SLICE_NALU             	= 4,
    H264_IDR_SLICE_NALU             	= 5,
    H264_SEI_NALU                   	= 6,
    H264_SPS_NALU                   	= 7,           
    H264_PPS_NALU                   	= 8,
    H264_AUD_NALU                   	= 9,
    H264_EOSEQUENCE_NALU            	= 10,
    H264_EOSTREAM_NALU              	= 11,
    H264_FILLER_NALU                	= 12
} H264NALUnitType;

typedef struct _H264Picture {
	IppPicture	pic;
    Ipp32u      nFrameNum;
	Ipp32s      nPicNum;
	Ipp32s      nLongtermPicNum;
	Ipp32s      nLontermFrameIdx;
} IppH264Picture;

typedef struct _IppH264PicList {
    IppPicture              *pPic;
    struct _IppH264PicList  *pNextPic;
} IppH264PicList;

typedef struct _H264EncoderParSet {
	/* Level 1 (must set) */
	int         iWidth;                         /* QCIF: 176*144, CIF: 352*288, QVGA: 320*240, VGA: 640*480                             */
	int         iHeight;                        /* QCIF: 176*144, CIF: 352*288, QVGA: 320*240, VGA: 640*480                             */
    int         iFrameRate;                     /* typical value: 30 or 15                                                              */
    int         iPBetweenI;                     /* the number of P frames between two successive intra frames                           */
    int         iQpFirstFrame;                  /* [10, 51], small value means high bitrate, valid when bRCEnable=0                     */
	int         bRCEnable;                      /* 1: enable, 0: disable                                                                */
	int         iRCBitRate;                     /* (bits/second), valid when bRCEnable=1                                                */
	int         iRCMaxBitRate;                  /* (bits/second), valid when bRCEnable=1, >= 1.5 * iRCBitRate                           */
	int			iDelayLimit;                    /* [500, 2000](ms), valid when bRCEnable=1                                              */
    int         nQualityLevel;                  /* 0: customize, 1: performance biased, 2: compromise, 3: quality biased                */

    /* Level 2 (valid only when nQualityLevel = 0)*/
    /* general */
    int         levelIdc;                       /* 10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51, typical value: 30        */
    int         iQcIndexOffset;                 /* [-12, 12], typical value: 0                                                          */
    /*loopfilter*/
    int         bDeblockEnable;                 /* 1: enable, 0: disable                                                                */
    int         disableDeblockFilterIdc;        /* 0 and 2 valid when bDeblockEnable=1, typical value: 0; 1 valid when bDeblockEnable=0 */
    int         iLoopFilterAlphaC0OffsetDiv2;   /* [-6, 6], typical value: 0, valid when bDeblockEnable=1                               */
    int         iLoopFilterBetaOffsetDiv2;      /* [-6, 6], typical value: 0, valid when bDeblockEnable=1                               */
    /*intra*/
    int         bConstrainedIntraPredFlag;      /* 0: disable, 1: enable                                                                */
    int         bIntra4x4PredModeEnable;        /* 0: disable, 1: enable                                                                */
    /*inter*/
    int         b8x8BlockSplitEnable;           /* 0: disable, 1: enable (16x8, 8x16, 8x8)                                              */
    int         b4x4BlockSplitEnable;           /* 0: disable, 1: enable (8x4, 4x8, 4x4)                                                */
    int			bSubPixelSearchFastMode;        /* 0: disable, 1: enable                                                                */
    int         i16x16SearchRange;              /* typical value: 16                                                                    */
    int         nSubPixelRefineLevel;           /* 0: disable, 1: half pixel, 2: quarter pixel, 3: each mode&quarter pixel              */
    int         nModeDecisionEarlyStopEnable;   /* 0: disable, 1: enable                                                                */ 

    /* Level 3, reserved, don't need set*/
	void*       pUserEncoderParam;
    int         iPictureComplexity;
    int         iSliceLength;
    int         bHardmardEnable;
    int         iRCType;
    int			bHalfPixelSearchEnable;
    int         bQuarterPixelSearchEnable;
    int         i8x8SearchRange;
    int         i4x4SearchRange;
    H264ProfileIdc profileIdc;
    int         nNumberReferenceFrames;
    int         nNumSliceGroups;
    int         nFMOType;
        
} H264EncoderParSet;

//typedef IppCodecStatus (*IppH264Callback)();
typedef struct _H264EncoderCBTable
{
   /* memory blocks malloc callback */
   MiscCallocCallback h264MemCalloc;
   /* memory blocks calloc callback */
   MiscFreeCallback   h264MemFree;
} H264EncoderCBTable;

typedef struct _H264NALUnitList {
	H264NALUnitType          nUnitType;
	Ipp8u*                   pUnitstream;
	int                      nUnitLength;
	struct _H264NALUnitList* pNextNALUnit;
} H264NALUnitList;

/* Frame pad width */
#define     IPP_H264_LUM_PAD_WIDTH  		32
#define     IPP_H264_CHR_PAD_WIDTH  		16

/***** Multimedia Codec Functions *************************************************/

/***** Video Codecs *****/



/* IPP Stabilizer plug-in, used as callbacks for IPP codecs only! */


IPPCODECAPI(IppCodecStatus, appiInitAlloc_IS, (
	void* , void** 
))

IPPCODECAPI(IppCodecStatus, appiFree_IS, (
	void** 
))

IPPCODECAPI(IppCodecStatus, appiPreprocess_IS, (
	IppPicture *,
	IppMotionVector* , IppMotionVector* ,
	int , void *
))

IPPCODECAPI(IppCodecStatus, appiPostprocess_IS, (
	IppPicture *,
	IppMotionVector* , Ipp32s* ,
	void* 
))


/* MPEG4 Decoder */


IPPCODECAPI( IppCodecStatus, DecoderFree_MPEG4Video, (
	void **ppDecoderState
))

IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_MPEG4Video, (
	IppBitstream *pBitStream, int *pWidth, int *pHeight,
	MiscGeneralCallbackTable *pCallbackTable,
	void **ppDecoderState
))

IPPCODECAPI( IppCodecStatus, Decode_MPEG4Video, (
	IppBitstream *pBitStream, IppPicture *pPicture,
	void *pDecoderState,
	int bLastOutputFrame
))

IPPCODECAPI (IppCodecStatus, DecodeSendCmd_MPEG4Video, (
    	int	cmd,
		void *pInParam,
    	void *pOutParam,
    	void *pDecoderState
))

/**** MPEG-4 Encoder ****/


IPPCODECAPI( IppCodecStatus, EncoderInitAlloc_MPEG4Video, (
	IppBitstream *pBitStream, IppParSet *pParInfo, 
	MiscGeneralCallbackTable *pCallbackTable,
	IppPicture *pPicture,
	void ** ppEncoderState
))


IPPCODECAPI( IppCodecStatus, EncoderFree_MPEG4Video, (
			void **ppEncoderState
))


IPPCODECAPI( IppCodecStatus, Encode_MPEG4Video, (
    IppBitstream *pBitStream,
    IppPicture *pPicture,
    void *pEncoderState
))

IPPCODECAPI (IppCodecStatus, EncodeSendCmd_MPEG4Video, (
    	int	cmd,
		void *pInParam,
    	void *pOutParam,
    	void *pEncoderState
))

/**** H263 Decoder ****/

IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_H263Video, (
	IppBitstream *pBitstream, MiscGeneralCallbackTable *pCallbackTable,
	void ** ppDecoderState
))

IPPCODECAPI( IppCodecStatus, DecoderFree_H263Video, (
	 void **ppDecoderState
))

IPPCODECAPI( IppCodecStatus, Decode_H263Video, (
	 IppBitstream *pBitstream,
	 IppPicture *pDstPicture, 
	 void *pDecoderState
))

IPPCODECAPI (IppCodecStatus, DecodeSendCmd_H263Video, (
    	int	cmd,
		void *pInParam,
    	void *pOutParam,
    	void *pDecoderState
))

/**** H263 Encoder ****/

IPPCODECAPI( IppCodecStatus, EncoderInitAlloc_H263Video, (
	IppH263ParSet *pParInfo, MiscGeneralCallbackTable *pCallbackTable, 
	IppPicture *pPicture, void ** ppEncoderState
))

IPPCODECAPI( IppCodecStatus, EncoderFree_H263Video, (
	 void **ppEncoderState
))

IPPCODECAPI( IppCodecStatus, Encode_H263Video, (
	 IppBitstream *pBitstream,
	 IppPicture *pDstPicture, 
	 void *pEncoderState
))

IPPCODECAPI (IppCodecStatus, EncodeSendCmd_H263Video, (
    	int	cmd,
		void *pInParam,
    	void *pOutParam,
    	void *pEncoderState
))

//remove freeze to example code
IPPCODECAPI( IppCodecStatus, Encode_FillFreezeH263Video, (
	 IppBitstream *pBitstream,
	 void *pEncoderState
))
//remove freeze to example code

/**** H264 Decoder ****/
IPPCODECAPI( IppCodecStatus, DecoderInitAlloc_H264Video, (
    MiscGeneralCallbackTable *pSrcCallbackTable, 
    void **ppDstDecoderState
))

IPPCODECAPI( IppCodecStatus, DecodeFrame_H264Video, (
    IppBitstream *pSrcBitStream, 
    IppH264PicList **ppDstPicList,
    void *pSrcDstDecoderState, 
    int *pDstNumAvailFrames
))

IPPCODECAPI( IppCodecStatus, DecoderFree_H264Video, (
    void **ppSrcDecoderState
))

IPPCODECAPI( IppCodecStatus, DecoderUpdate_H264Video, (
    void *pSrcDstDecoderState
))

IPPCODECAPI (IppCodecStatus, DecodeSendCmd_H264Video, (
    	int	cmd,
		void *pInParam,
    	void *pOutParam,
    	void *pDecoderState
))

/**** H264 Encoder ****/
IPPCODECAPI( IppCodecStatus, EncoderInitAlloc_H264Video, (
    H264EncoderCBTable *pSrcCallbackTable,
	H264EncoderParSet  *pSrcParInfo,
	IppBitstream	   *pSrcDstStream,
	void	           **ppDstEncoderState,
    H264NALUnitList    **ppDstNALUnitList
))

IPPCODECAPI( IppCodecStatus, EncoderFree_H264Video, (
    H264EncoderCBTable    *pSrcCallbackTable,
    void                  **ppSrcEncoderState,
    H264NALUnitList       **ppSrcNALUnitList
))

IPPCODECAPI( IppCodecStatus, EncodeFrame_H264Video, (
    H264EncoderCBTable    *pSrcCallbackTable,
    H264EncoderParSet     *pSrcParInfo,
    IppBitstream          *pSrcDstBitStream,
    IppPicture            *pSrcPicture,
    void                  *pSrcDstEncoderState,
    H264NALUnitList       *pDstNALUnitList,
	IppPicture            **pRecPicture
))

IPPCODECAPI( IppCodecStatus, EncodeSendCmd_H264Video, (
	int cmd, 
	void *pInParam, 
	void *pOutParam, 
	void *pEncoderState
))

typedef void* (*FrameMallocCallback)(int size, int align, void* pUsrObj);             //function return NULL to indicate memory allocation fail
typedef void  (*FrameFreeCallback)(void *p, void* pUsrObj);
typedef struct _FrameMemOpSet {
                FrameMallocCallback     fMallocFrame;
                FrameFreeCallback       fFreeFrame;
                void*                   pUsrObj;

} FrameMemOpSet;


/**** MPEG2 Main Profile Decoder ****/
/*Initialize working buffer and global decoder state*/
IPPCODECAPI(IppCodecStatus, DecoderInitAlloc_MPEG2Video,(
				MiscGeneralCallbackTable	*pSrcCallbackTable,
				void						**ppDstDecoderState,
				IppBitstream				*pBitstream));

/*Decode picture*/
IPPCODECAPI(IppCodecStatus, Decode_MPEG2Video,(
				IppBitstream				*pBitstream,
				IppPicture					*pDstPicture,
				Ipp32u						bNoMoreStream,
				void						*pDecoderState));	

IPPCODECAPI(IppCodecStatus, DecodeSendCmd_MPEG2Video,(
				Ipp32s						cmd,
				void						*pInParam,
				void						*pOutParam,
				void						*pDecoderState));
/*Destory buffers*/
IPPCODECAPI(IppCodecStatus, DecoderFree_MPEG2Video,(
				void						**ppState));


/* vmeta */
#define VMETA_MAX_STRM_BUF_NUM       32
#define VMETA_MAX_DIS_BUF_NUM        32
#define VMETA_STRM_BUF_ALIGN         1024
#define VMETA_DIS_BUF_ALIGN          4096
#define VMETA_BUF_PROP_CACHEABLE     0x1

typedef enum _IppVideoStreamFormat {
    IPP_VIDEO_STRM_FMT_MPG1		            = 0,
    IPP_VIDEO_STRM_FMT_MPG2                 = 1,
    IPP_VIDEO_STRM_FMT_MPG4                 = 2,
    IPP_VIDEO_STRM_FMT_H261                 = 3,
    IPP_VIDEO_STRM_FMT_H263                 = 4,
    IPP_VIDEO_STRM_FMT_H264		            = 5,
    IPP_VIDEO_STRM_FMT_VC1		            = 6,
    IPP_VIDEO_STRM_FMT_JPEG		            = 7,
    IPP_VIDEO_STRM_FMT_MJPG		            = 8,
    IPP_VIDEO_STRM_FMT_VP6		            = 9,
    
    IPP_VIDEO_STRM_FMT_UNAVAIL              = 0x7fffffff,
}IppVideoStreamFormat;

typedef enum _IppVmetaBufferType {
    IPP_VMETA_BUF_TYPE_STRM		            = 0,
    IPP_VMETA_BUF_TYPE_PIC	                = 1,
    
    IPP_VMETA_BUF_TYPE_UNAVAIL	            = 0x7fffffff
}IppVmetaBufferType;

typedef enum _IppVmetaBitstreamFlag {
    IPP_VMETA_STRM_BUF_PART_OF_FRAME        = 0,
    IPP_VMETA_STRM_BUF_END_OF_UNIT          = 1,

    IPP_VMETA_STRM_BUF_UNAVAIL              = 0x7fffffff
}IppVmetaBitstreamFlag;

typedef struct _IppVmetaDecParSet {
    IppVideoStreamFormat    strm_fmt;           /*stream format, such as h264, mpeg2, etc*/
    IppColorFormat          opt_fmt;            /*output format, such as yuv422i*/
    Ipp32u  	            no_reordering;      /*no reordering operation inside codec*/
    Ipp32u                  pp_hscale;          /*Horizontal down-scaling ratio (JPEG only),    
                                                0 or 1: no scaling, 2: 1/2, 4: 1/4, */
    Ipp32u                  pp_vscale;          /*Vertical down-scaling ratio (JPEG only)
                                                0 or 1: no scaling, 2: 1/2, 4: 1/4, 8: 1/8*/
} IppVmetaDecParSet;

typedef struct _IppVideoSeqInfo {
    Ipp32u      is_intl_seq;            // Nonzero if it is a field-picture sequence.
    Ipp32u      max_width;              // Maximum coded picture width (in pixels) of the whole sequence.
    Ipp32u      max_height;             // Maximum coded picture height (in pixels) of the whole sequence.
    Ipp32u      dis_buf_size;           // Size (in bytes) of the display frame buffer.
    Ipp32u      dis_buf_num;            // Informative. Number of display frame buffers required by reordering.
    Ipp32u      dis_stride;             // Stride (in bytes) of the display frame buffer.
    Ipp32u      frame_rate_num;         // Numerator of the frame rate, if available.
    Ipp32u      frame_rate_den;         // Denominator of the frame rate, if available.
}IppVideoSeqInfo;

typedef struct _IppVmetaDecInfo {
    IppVideoSeqInfo seq_info;
}IppVmetaDecInfo;

typedef struct _IppVmetaBitstream {
    Ipp8u           *pBuf;          /*point to actual block of memory*/
    Ipp32u          nPhyAddr;       /*physical address of pBuf*/
    Ipp32u          nBufSize;       /*allocated buffer size*/
    Ipp32u          nDataLen;       /*the length of filled data*/
    Ipp32u          nOffset;        /*start offset of valid data in bytes from the start of the buffer, reserved, should be set to 0*/
    Ipp32u          nBufProp;       /*property of the buffer memory: dma, non-cacheable, reserved*/
    Ipp32u          nFlag;          /*information for data in buffer, such as end of frame*/
    void            *pIntData0;     /*for internally used, reserved*/
    void            *pIntData1;     /*for internally used, reserved*/
    void            *pUsrData0;     /*pointer to user private data, not used inside codec*/
    void            *pUsrData1;     /*pointer to user private data, not used inside codec*/
    void            *pUsrData2;     /*pointer to user private data, not used inside codec*/
    void            *pUsrData3;     /*pointer to user private data, not used inside codec*/
}IppVmetaBitstream;

typedef struct _IppPicDataInfo {
    Ipp32u          pic_type;           /*0:non-interlaced frame, 1: paired field, 2: interlaced frame, 3: non-paired field*/
    Ipp32u          is_correct[2];      /*if the decoded pic is correct*/
    Ipp32u          chksum_data[2][8];  /*chksum of frame or field for validation*/
    Ipp32s          poc[2];             /*picture order count of the picture*/
    Ipp32u          coded_pic_idx[2];   /*picture index in decoding order*/
    Ipp32u          is_btm;             /*only valid for pic_type == 3*/
}IppPicDataInfo;

typedef struct _IppVideoPicture {
    Ipp8u           *pBuf;          /*point to actual block of memory*/
    Ipp32u          nPhyAddr;       /*physical address of pBuf*/
    Ipp32u          nBufSize;       /*allocated buffer size*/
    Ipp32u          nDataLen;       /*the length of filled data*/
    Ipp32u          nOffset;        /*start offset of valid data in bytes from the start of the buffer */
    Ipp32u          nBufProp;       /*property of the buffer memory: dma, non-cacheable, reserved*/
    Ipp32u          nFlag;          /*information for data in buffer, reserved*/
    IppPicture      pic;            /*picture descriptor, including plane number, format, roi, etc*/
    IppPicDataInfo  PicDataInfo;    /*additional information on picture data*/
    void            *pIntData0;     /*for internally used, reserved*/
    void            *pIntData1;     /*for internally used, reserved*/
    void            *pUsrData0;     /*pointer to user private data, not used inside codec*/
    void            *pUsrData1;     /*pointer to user private data, not used inside codec*/
    void            *pUsrData2;     /*pointer to user private data, not used inside codec*/
    void            *pUsrData3;     /*pointer to user private data, not used inside codec*/
}IppVmetaPicture;

IPPCODECAPI (IppCodecStatus, DecodeSendCmd_Vmeta, (
             int	cmd,
             void 	*pInParam,
             void 	*pOutParam,
             void 	*pSrcDecoderState
             ))

IPPCODECAPI(IppCodecStatus, DecoderInitAlloc_Vmeta, (
            IppVmetaDecParSet           *pVmetaDecParSet,
            MiscGeneralCallbackTable    *pSrcCallbackTable,
            void                        **ppDstDecoderState
            ))

IPPCODECAPI(IppCodecStatus, DecodeFrame_Vmeta, (
            IppVmetaDecInfo             *pDecInfo,      
            void                        *pSrcDstDecoderState 
            ))


IPPCODECAPI(IppCodecStatus, DecoderFree_Vmeta, (
            void                        **pSrcDecoderState
            ))


IPPCODECAPI(IppCodecStatus, DecoderPushBuffer_Vmeta, (
            IppVmetaBufferType  nBufType,
            void                *pBufPtr,
            void                *pSrcDecoderState
            ))
IPPCODECAPI(IppCodecStatus, DecoderPopBuffer_Vmeta, (
            IppVmetaBufferType  nBufType,
            void                **ppBufPtr,
            void                *pSrcDecoderState
            ))

typedef struct _IppVmetaEncParSet {
    /* Level 1 (must set) */
    IppColorFormat          eInputYUVFmt;
    IppVideoStreamFormat    eOutputStrmFmt;
    int                     nWidth;             /* QCIF: 176*144, CIF: 352*288, QVGA: 320*240, VGA: 640*480                             */
    int                     nHeight;            /* QCIF: 176*144, CIF: 352*288, QVGA: 320*240, VGA: 640*480                             */
    int                     nPBetweenI;         /* the number of P frames between two successive intra frames                           */    
    int                     bRCEnable;          /* 0:disable, 1:enable                                                                  */
    int                     nQP;                /* small value means high bitrate, valid when bRCEnable=0                               */
    int                     nRCBitRate;         /* (bits/second), valid when bRCEnable=1                                                */
    int                     nFrameRateNum;      /* typical value: 30 or 15                                                              */
} IppVmetaEncParSet;

typedef struct _IppVmetaEncInfo {
    Ipp32u  dis_buf_size;
}IppVmetaEncInfo;


IPPCODECAPI (IppCodecStatus, EncodeSendCmd_Vmeta, (
             int	cmd,
             void 	*pInParam,
             void 	*pOutParam,
             void 	*pSrcEncoderState
             ))

 IPPCODECAPI(IppCodecStatus, EncoderInitAlloc_Vmeta, (
             IppVmetaEncParSet           *pVmetaEncParSet,
             MiscGeneralCallbackTable    *pSrcCallbackTable,
             void                        **ppDstEncoderState
             ))

 IPPCODECAPI(IppCodecStatus, EncodeFrame_Vmeta, (
             IppVmetaEncInfo             *pEncInfo,      
             void                        *pSrcDstEncoderState 
             ))


 IPPCODECAPI(IppCodecStatus, EncoderFree_Vmeta, (
             void                        **pSrcEncoderState
             ))


 IPPCODECAPI(IppCodecStatus, EncoderPushBuffer_Vmeta, (
             IppVmetaBufferType  nBufType,
             void                *pBufPtr,
             void                *pSrcEncoderState
             ))

 IPPCODECAPI(IppCodecStatus, EncoderPopBuffer_Vmeta, (
             IppVmetaBufferType  nBufType,
             void                **ppBufPtr,
             void                *pSrcEncoderState
             ))
#ifdef __cplusplus
}
#endif

#endif    /* #ifndef _CODECVC_H_ */

/* EOF */


